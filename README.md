# Ryutai - 2D Fluid Simulation ( WIP )

<div align="center">
<table>
  <tbody>
    <tr>
      <td >
        <img src="https://raw.githubusercontent.com/irrevocablesake/Ryutai/refs/heads/master/images/Initial.png" width="100%">
        <p style="text-align: center; font-style: italic; font-size: 14px; color: #555;">
          Start
        </p>
      </td>
      <td>
        <img src="https://raw.githubusercontent.com/irrevocablesake/Ryutai/refs/heads/master/images/Later.png" width="100%">
        <p style="text-align: center; font-style: italic; font-size: 14px; color: #555;">
          Simulation
        </p>
      </td>
    </tr>
  </tbody>
</table>
</div>

## Intuition
Fluids are everywhere, water, honey, air, well anything that flows falls in that category. The goal was to implement a **2D Fluid** Simulation using **C++** and **Vulkan**, and later expand it into **3D Fluid** Simulation. All of this is based on **Jos Stam Real-Time Fluid Dynamics** Technique. 

So far with the progress, we start with a velocity field initialized Perlin Noise, using this velocity field we transport the density field. In between there are multiple passes to simulate proper flow of fluids with respect to laws of physics, since this is WIP - the physics is wonky but constant tweaks are being made so it becomes a better simulation.

# Simulation
Here is a video of how the simulation would proceed from an initial state to a few seconds into the simulation

https://github.com/user-attachments/assets/7a25223a-779c-4859-8049-021a4541ed51

## Resources
- [ Fluid Dynamics NVIDIA ](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu)
- [ Fluid Simulation for Dummies ]( https://www.mikeash.com/pyblog/fluid-simulation-for-dummies.html )
- [ Coding Train - Fluid Simulation ]( https://www.youtube.com/watch?v=alhpH6ECFvQ )

