# CarScan
Playing with a 3D scan of a "car wreck".

WebGL: https://mrbid.github.io/carscan/

The 3D scan is from [matousekfoto](https://sketchfab.com/matousekfoto) and is available for free from here [Green car wreck.](https://sketchfab.com/3d-models/green-car-wreck-a5b233dfe0024ff0b9d33f5469b10dc8)

I've baked the texture maps to vertex colors and rendered this with phong using the diffuse map as the specular map, I tried to remove the saturation from the diffuse vertex colors which worked fine but then trying to use the vertex color brush in Blender to darken or lighten certain areas of the grayscale specular map was far too unresponsive to be practical hence settling on just using the diffuse map also as the specular, reduces file size a little and honestly doesn't look too bad.

The general idea here is to use this as some sort of collision mesh, all the vertices are about evenly spaced meaning I can do simple sphere collision detection on each vertex to simulate some basic physics interation of balls raining down on the mesh or similar.
