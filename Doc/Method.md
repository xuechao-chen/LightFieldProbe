# Method

## Model Simplification

CJW

SIGGRAPH

## One-pass Rasterization 

Single Input(simplified model) and Multiple Output(position maps from probe->merge into one map)

## Reconstruct Irradiance/Radiance Map

Based on position maps from probe and gbuffer from light

## Shading with Reconstructed Irradiance Map

Similar as method in Light Field Probe GI

## Parallel Texel Fetching

PYL

## Future Work

Glossy(non-mirror like) Effect



## Schedule

```mermaid
gantt
       dateFormat  MM-DD
       title I3D 2020 Schedule(Deadline 12-13)

	   section Idea Implementation
	   HighRes Irradiance Map Reconstruction :done,   task1, 11-13
	   LowRes Irradiance Map Reconstruction  :done,   task2, after task1, 1d
	   Complex Scene Reconstruction          :active, task3, after task1, 1d
	   Multiple Light Reconstruction         :        task4, after task3, 1d
       Code Refactor(Different Light/Scene)  :        task5, after task3, 1d
       
       section Performance Optimization
       Model Simplification                  :        task6, after task5, 2d
       One-pass Rasterization                :        task7, after task5, 5d
       Parallel Texel Fetching               :        task8, after task5, 3d
       Drawcall Analysic(Nsight)             :        task9, after task7, 3d
       OpenGL Implementation(Maybe)          :        task10,after task9, 3d
       
       section DDGI Implementation
       Figure out Algorithm Details          :        task11,after task5, 2d
       RayTracing Implementation             :        task12,after task11,4d
       Dynamic Diffuse Lighting Effect       :        task13,after task12,5d
       
       section Writing Paper
       Writing Paper                         :        task14,after task13, 1d
       
       section Experiments
       Experiments                           :        task15,after task13, 1d
       

```

