# Ryutai - 2D Fluid Simulation ( WIP )

<div align="center">
<table>
  <tbody>
    <tr>
      <td>
        <img src="https://raw.githubusercontent.com/irrevocablesake/Ryutai/refs/heads/master/images/repo-banner.png" width="100%">
        <p style="align: center; font-style: italic; font-size: 14px; color: #555;">
          Navier Stokes - Incompressible Fluid Simulation
        </p>
      </td>
    </tr>
  </tbody>
</table>
</div>

## Intuition
Ryutai - Japanese word for "fluid"

Fluids are everywhere, water, honey, air, well anything that flows falls in that category. The goal was to implement a **2D Fluid** Simulation using **C++** and **Vulkan**, and later expand it into **3D Fluid** Simulation all based on **Jos Stam Real-Time Fluid Dynamics** Technique, which itself uses Navier Stokes Incompressible Fluid Equations. 

The equations itself look very complicated at a first glance, but - [ Fluid Dynamics NVIDIA ](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu) does a great job at breaking the equations down to chewable bites. Well, I said chewable not digestable haha, it helped me atleast understand what's happening overall within the simulation

The idea is quite simple: Fluids flow, what do they flow? They flow things that are present in them, and more importantly - they flow themselves too. 

At the highest level we can think of this simulation made up of two things:
-  Velocity Field: A field that tells you where the fluid is flowing
-  Dye Field: A field that tells you if the fluid flows something in it, this is like dropping paint drops in a bucket of water, it gives us the ability to visualize the flow properly.

The rest of the algorithm works to make sure the velocity field is computed according to the equations, hence keeping it true to Physics. A general flow of the program can be thought as follows:
-  We start with a zero velocity field and a zero dye field ( no flow & no color)
-  We introduce a few "splats" these disturb these fields:
  - eg. A splat is made up of radius, color and location, we send this information to the velocity field and dye field, they accordingly make a note of this splat
  - Velocity field reads the color component as force
  - Dye Field, reads the color and stores it
- But this "disturbing" of fluid due to splats, causes the velocity field to have some "bad areas", areas, that compress or expand, that wouldn't respect physics.
- The first thing after splats have been introduced is to find these "bad areas"
- Once we have those bad areas, we need to compute a pressure that would counteract these "bad areas", which makes the simulation "incompressible"
- We fix the velocity using the pressure, and now we have a stable velocity field again
- The most important thing a fluid does is, and that's the reason for it's mere existance: it flows. We now use our stable velocity to do two things: flow velocity itself and whatever is present in the fluid ( dye )
- We also ensure to dampen the flow, so eventually as the simulation keeps running, the flow looses energy and comes to standstill ( at that point we just kill the dye )
- We update all the fields to hold the latest outputs, we pass these outputs to next frames, and thus the algorithm keeps simulating.

So far with the progress, we start with a velocity field initialized using Perlin Noise, with this velocity field we transport the density field. In between there are multiple passes to simulate proper flow of fluids with respect to laws of physics, since this is WIP - the physics is wonky but constant tweaks are being made so it becomes a better simulation.

# Simulation
Here is a video of how the simulation would proceed from an initial state to a few seconds into the simulation

https://github.com/user-attachments/assets/a475feb6-10c5-4e8b-bf70-4d85156f5068

## Resources
- [ Fluid Dynamics NVIDIA ](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu)
- [ Fluid Simulation for Dummies ]( https://www.mikeash.com/pyblog/fluid-simulation-for-dummies.html )
- [ Coding Train - Fluid Simulation ]( https://www.youtube.com/watch?v=alhpH6ECFvQ )
- [ WebGL Fluid Simulation ]( https://github.com/PavelDoGreat/WebGL-Fluid-Simulation/tree/master )

