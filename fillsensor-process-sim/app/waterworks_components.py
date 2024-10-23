import random
import asyncio
from second_order_system import pt2_system_step
from math import sqrt

class tank:
    def __init__(self, name, volume, height, max_fill_level, min_fill_level, fill_level, url, sim_step=.1):
        self.name = name
        self.volume = volume*(10**3) # convert to l
        self.height = height # in mm
        self.max_fill = max_fill_level # in mm
        self.min_fill = min_fill_level # in mm
        self.fill_level = fill_level # in mm
        self.fill_percentage = (self.fill_level) / self.height
        self.url = url
        self.sim_step = sim_step
        self.client = None

    def calculate_fill_volume(self, fill_level):
        # calculate fill volume in l from fill level in mm
        fill_volume = self.volume * (fill_level / self.height)
        return fill_volume
    
    def calculate_fill_level(self, fill_volume):
        # calculate fill level in mm from fill volume in l
        fill_level = (fill_volume / self.volume) * self.height
        return fill_level
    
    def calculate_new_fill_level(self, inflows, outflows):
        # calculate new fill level in mm from sensor reading in mm, inflows in l/s and outflows in l/s
        # flows with ambigous directions should be added to inflows with positive sign if headed towards tank
        total_inflow = 0
        for inflow in inflows:
            total_inflow += inflow
        total_outflow = 0
        for outflow in outflows:
            total_outflow += outflow
        fill_volume = self.calculate_fill_volume(self.fill_level)
        new_fill_volume = fill_volume + (inflow - outflow) * self.sim_step
        new_fill_level = self.calculate_fill_level(new_fill_volume)
        self.fill_level = new_fill_level
        self.fill_percentage = (self.fill_level) * 100 / self.height
        new_fill_level_meassured = new_fill_level + random.gauss(0, 10)
        return new_fill_level_meassured
    
class pump:
    def __init__(self, name, nominal_flow_rate, flow_destination, url = "", sim_step=.1):
        self.name = name
        self.url = url # IP address of OpenPLC Modbus server controlling the pump
        self.pump_status = False # False = off, True = on
        self.nominal_flow_rate = nominal_flow_rate # in l/s
        self.current_flow_rate = 0
        self.d_current_flow_rate = 0
        self.flow_destination = flow_destination
        self.sim_step = sim_step
        self.K = 1.0         # Gain
        self.zeta = 0.7      # Damping ratio
        self.tau = 0.1       # Time constant
        self.dt = 0.01       # Time step size
        self.client = None


    def get_flow_PT2(self):
        # Return meassured outflow in l/s and store real outflow in self.current_flow_rate
        # Simplified model: outflow responds to pump status change with a second order system
        error =  random.gauss(0, 5) * (self.current_flow_rate/self.nominal_flow_rate)
        if self.pump_status == True:
            input_signal = self.nominal_flow_rate
        else:
            input_signal = 0
        # Calculate euler steps for PT2 system
        euler_steps = int(self.sim_step/self.dt)
        for i in range(euler_steps):
            next_state = pt2_system_step([self.current_flow_rate, self.d_current_flow_rate], input_signal, self.dt, self.K, self.zeta, self.tau)
            self.current_flow_rate = next_state[0]
            self.d_current_flow_rate = next_state[1]
        return abs(self.current_flow_rate+error)
    
class well:
    def __init__(self, name, volume, height, max_fill_level, min_fill_level, fill_level, ip_address = "172.18.0.3", sim_step=.1):
        self.name = name
        self.volume = volume*(10**3) # convert to l
        self.height = height # in mm
        self.max_fill = max_fill_level # in mm
        self.min_fill = min_fill_level # in mm
        self.drill_depth = 100 # H in m
        self.ip_address = ip_address
        self.sim_step = sim_step
        self.fill_level = fill_level # in mm
        self.client = None

    def set_hysteresis(self, client, max_fill_level, min_fill_level):
        self.max_fill = max_fill_level
        self.min_fill = min_fill_level
        print(f'Hysteresis set to: {self.max_fill}mm - {self.min_fill}mm')
    
    def calculate_fill_volume(self, fill_level):
        # calculate fill volume in l from fill level in mm
        fill_volume = self.volume * ((self.height - fill_level) / self.height)
        return fill_volume
    
    def calculate_fill_level(self, fill_volume):
        # calculate fill level in mm from fill volume in l
        fill_level = self.height - ((fill_volume / self.volume) * self.height)
        return fill_level
    
    def calculate_inflow(self):
        # calculate inflow in l/s from fill level in mm
        # simplified model based on Darcy's law
        kf = 97.22 # faktor to scale inflow to l/s
        delta2groundwater = (self.fill_level+0.001)/1000 # s in m
        inflow = kf * delta2groundwater/self.drill_depth # in l/s
        return inflow
    
    def calculate_new_fill_level(self, inflows, outflows):
        # calculate new fill level in mm from sensor reading in mm, inflows in l/s and outflows in l/s
        # flows with ambigous directions should be added to inflows with positive sign if headed towards tank
        total_inflow = 0
        for inflow in inflows:
            total_inflow += inflow
        total_outflow = 0
        for outflow in outflows:
            total_outflow += outflow
        fill_volume = self.calculate_fill_volume(self.fill_level)
        new_fill_volume = fill_volume + (inflow - outflow) * self.sim_step
        new_fill_level = self.calculate_fill_level(new_fill_volume)
        self.fill_level = new_fill_level
        new_fill_level_meassured = new_fill_level + random.gauss(0, 10)
        return new_fill_level_meassured
    

class withdrawal_point:
    def __init__(self, name):
        self.name = name
        self.consumption = 0 # in l/s
        self.withdrawal_mode = 'constant'

async def initialize_PLCs(HB, Brunnen, pump_1):
    await HB.send_maximal_fill_level(HB.max_fill)
    await HB.send_minimal_fill_level(HB.min_fill)
    await HB.send_fill_level_sensor_reading(HB.fill_level)
    await Brunnen.send_maximal_fill_level(Brunnen.max_fill)
    await Brunnen.send_minimal_fill_level(Brunnen.min_fill)
    await Brunnen.send_fill_level_sensor_reading(Brunnen.fill_level)
    await pump_1.send_pump_request_well(False)
    await pump_1.send_pump_request_tank(False)
