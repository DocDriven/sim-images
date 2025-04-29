import random
from second_order_system import pt2_system_step

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
        return self.volume * (fill_level / self.height)
    
    def calculate_fill_level(self, fill_volume):
        # calculate fill level in mm from fill volume in l
        return (fill_volume / self.volume) * self.height
    
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
        new_fill_volume = fill_volume + (total_inflow - total_outflow) * self.sim_step
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
        self.current_flow_rate = 0.
        self.d_current_flow_rate = 0.
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
        for _ in range(euler_steps):
            next_state = pt2_system_step((self.current_flow_rate, self.d_current_flow_rate), input_signal, self.dt, self.K, self.zeta, self.tau)
            self.current_flow_rate = next_state[0]
            self.d_current_flow_rate = next_state[1]
        return abs(self.current_flow_rate+error)
