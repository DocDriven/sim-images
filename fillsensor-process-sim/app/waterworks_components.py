import random
from typing import Tuple


class Tank:
    def __init__(self,
                 name: str,
                 volume_m3: float,
                 height_mm: float,
                 max_lvl_mm: float,
                 min_lvl_mm: float,
                 lvl_mm: float,
                 sim_step_s: float =.1):
        self.name = name
        self.volume_l = 1_000 * volume_m3
        self.height_mm = height_mm
        self.max_lvl_mm = max_lvl_mm
        self.min_lvl_mm = min_lvl_mm
        self.lvl_mm = lvl_mm
        self.sim_step = sim_step_s
        self.update_fill_pct()

    def update_fill_pct(self):
        """Update fill percentage from current fill level."""
        self.fill_pct = 100 * self.lvl_mm / self.height_mm

    def mm_to_l(self, lvl_mm: float) -> float:
        """Calculate fill volume in liters from fill level in mm."""
        return self.volume_l * (lvl_mm / self.height_mm)
    
    def l_to_mm(self, volume_l: float) -> float:
        """Calculate fill level in mm from fill volume in liters."""
        return self.height_mm * (volume_l / self.volume_l)
    
    def calculate_new_fill_level(self, inflows: list[float], outflows: list[float]) -> float:
        """Update tank fill level based on inflows and outflows (both in liters/s)."""

        total_inflow = sum(inflows)
        total_outflow = sum(outflows)

        current_volume_l = self.mm_to_l(self.lvl_mm)
        new_volume_l = current_volume_l + (total_inflow - total_outflow) * self.sim_step

        new_lvl_mm = self.l_to_mm(new_volume_l)
        self.lvl_mm = new_lvl_mm
        self.update_fill_pct()

        # Add random sensor noise
        return new_lvl_mm + random.gauss(0, 10)


class Pump:
    def __init__(self,
                 name: str,
                 nominal_flow_rate_lps: float,
                 sim_step: float =.1):
        self.name = name
        self.pump_on = False
        self.nominal_flow_rate_lps = nominal_flow_rate_lps
        self.current_flow_rate_lps = 0.
        self.d_current_flow_rate_lps2 = 0.
        self.sim_step = sim_step
        self.K = 1.          # Gain
        self.zeta = 0.7      # Damping ratio
        self.tau = 0.1       # Time constant
        self.dt = 0.01       # Time step size

    @staticmethod
    def _pt2_system_step(
        s: Tuple[float, float],
        u: float,
        dt: float,
        K: float,
        zeta: float,
        tau: float
    ) -> Tuple[float, float]:
        """
        Calculate the next value of a second-order system (PT2) using the Euler method.

        Parameters:
            s: Current state [y(t), y'(t)]
            u: Input at the current time step
            dt: Time step size
            K: Gain
            zeta: Damping ratio
            tau: Time constant

        Returns:
            Next state [y(t+dt), y'(t+dt)]
        """
        y, dy = s

        # Calculate the next state using the PT2 system equation
        dydt = (1 / tau**2) * (u - 2*zeta*tau*dy - K*y)
        y_new = y + dt * dy
        dy_new = dy + dt * dydt
        return (y_new, dy_new)

    def get_flow(self) -> float:
        """Return measured outflow in liters/s and store real outflow internally.
        This simplified model responds to pump status changes with a second order system.
        """
        error =  random.gauss(0, 5) * self.current_flow_rate_lps / self.nominal_flow_rate_lps

        if self.pump_on == True:
            input_signal = self.nominal_flow_rate_lps
        else:
            input_signal = 0

        # Calculate euler steps for PT2 system
        n_euler_steps = int(self.sim_step / self.dt)
        for _ in range(n_euler_steps):
            next_state = Pump._pt2_system_step(
                (self.current_flow_rate_lps, self.d_current_flow_rate_lps2),
                input_signal,
                self.dt,
                self.K,
                self.zeta,
                self.tau)
            self.current_flow_rate_lps = next_state[0]
            self.d_current_flow_rate_lps2 = next_state[1]
        return abs(self.current_flow_rate_lps + error)
