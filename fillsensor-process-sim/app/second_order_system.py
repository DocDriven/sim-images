#import matplotlib.pyplot as plt

def pt2_system_step(y, u, dt, K, zeta, tau):
    """
    Calculate the next value of a second-order system (PT2) using the Euler method.
    
    Parameters:
        y: Current state [y(t), y'(t)]
        u: Input at the current time step
        dt: Time step size
        K: Gain
        zeta: Damping ratio
        tau: Time constant
    
    Returns:
        Next state [y(t+dt), y'(t+dt)]
    """
    y, dy = y  # Unpack the current state
    
    # Calculate the next state using the PT2 system equation
    dydt = (1 / tau**2) * (u - 2*zeta*tau*dy - K*y)
    y_new = y + dt * dy
    dy_new = dy + dt * dydt
    
    return [y_new, dy_new]


if __name__ == "__main__":
    # Example usage:
    K = 2.0         # Gain
    zeta = 0.85      # Damping ratio
    tau = .5       # Time constant
    dt = 0.01       # Time step size

    # Initial state and input
    initial_state = [0.0, 0.0]
    input_signal = 1.0

    ## Calculate the next state
    #next_state = pt2_system_step(initial_state, input_signal, dt, K, zeta, tau)
    #print("Next state:", next_state)

    time = [0.0]
    y = [initial_state[0]]
    dy = [initial_state[1]]
    next_state = initial_state

    for i in range(1000):
        next_state = pt2_system_step(next_state, input_signal, dt, K, zeta, tau)
        time.append(time[-1] + dt)
        y.append(next_state[0])
        dy.append(next_state[1])

    # plt.plot(time, y)
    # plt.ylabel('step response')
    # plt.show()




