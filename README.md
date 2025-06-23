# Gravitas

Gravitas is a real-time 3D gravitational simulation that visualizes celestial motion and mass-based spacetime distortion. Designed with C++, OpenGL, and ImGui, the project offers an interactive environment to explore the effects of gravity between planetary bodies.

## Important Note
This project is inspired by kavan010's Gravity_Sim project, and part of my code is his code, so please, credit kavan010 for the idea, like I am doing and will do!

## Secondary Note
The libs are already in the repo, but just don't forget to link them with my provided CMakeLists.txt!  

## 🌠 Features

- 🪐 **Realistic gravity-based motion** between celestial objects.
- 🔦 **Dynamic lighting** with Phong shading.
- ✨ **Glow effect** for stars (e.g., the Sun).
- 🧠 **ImGui interface** for:
  - Camera controls
  - Pausing/resuming simulation
  - Live object data monitoring
- 🌌 **Spacetime distortion grid** updated using relativistic principles.
- 🎮 Mouse and keyboard camera navigation.

## 🧪 Demo

![screenshot](https://user-images.githubusercontent.com/demo/gravity.gif)
> Simulation of a solar system-like setup with grid warping under mass influence.

## Math & Physics

Newton's Law of Universal Gravitation:
F = (G * m1 * m2) / r^2

Acceleration from Gravitational Force:
a = F/m

Schwarzschild Radius (for visualizing spacetime distortion):
rs = (2 * G * m) / c^2

v = v + a * dt
p = p + v * dt
