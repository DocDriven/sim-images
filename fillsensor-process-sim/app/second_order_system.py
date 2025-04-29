from typing import Tuple

def pt2_system_step(
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
    y, dy = s  # Unpack the current state
    
    # Calculate the next state using the PT2 system equation
    dydt = (1 / tau**2) * (u - 2*zeta*tau*dy - K*y)
    y_new = y + dt * dy
    dy_new = dy + dt * dydt
    
    return (y_new, dy_new)

