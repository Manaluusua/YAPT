
from py_yapt import Material, Light, vec3

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

plane_mesh = resources.load_meshes_from_path("models/plane.glb")[0][0][0]
sphere_mesh = resources.load_meshes_from_path("models/Sphere.glb")[0][0][0]
bunny_mesh = resources.load_meshes_from_path("models/bunny.glb")[0][0][0]

floor_mat = renderer.createMaterial("floor_mat")
floor_mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
floor_mat.setAlbedo(vec3([0.6, 0.6, 0.6]))
floor_mat.setRoughness(0.8)

gold_mat = renderer.createMaterial("gold_mat")
gold_mat.setFromMaterialPreset(Material.MaterialPreset.METAL_GOLD)
gold_mat.setRoughness(0.3)

plastic_mat = renderer.createMaterial("plastic_mat")
plastic_mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
plastic_mat.setAlbedo(vec3([0.8, 0.15, 0.1]))
plastic_mat.setRoughness(0.2)

def add_object(name, mesh, mat, pos, scale):
	obj = scene.create_render_object(name)
	obj.setMesh(mesh)
	obj.setMaterial(mat, 0)
	obj.getTransform().setTranslation(vec3(pos))
	obj.getTransform().setScale(vec3(scale))
	return obj

add_object("floor", plane_mesh, floor_mat, [0, 0, 0], 60)
add_object("sphere", sphere_mesh, gold_mat, [-6, 3, 0], 3)
add_object("bunny", bunny_mesh, plastic_mat, [5, 0, 0], 4)

def add_light(name, light_type, position, target, color, intensity):
	light = scene.create_light(name, light_type)
	light.getTransform().lookAt(vec3(position), vec3(target), vec3([0, 1, 0]))
	light.setColor(vec3(color))
	light.setIntensity(intensity)
	return light

#point and spot intensities are radiant intensity, the directional one is irradiance
add_light("point_light", Light.LightType.POINT, [-2, 10, 8], [0, 0, 0], [1.0, 0.85, 0.7], 100.0)
spot = add_light("spot_light", Light.LightType.SPOT, [12, 14, 6], [5, 0, 0], [0.7, 0.8, 1.0], 800.0)
spot.setSpotConeAngles(0.15, 0.35)
add_light("sun_light", Light.LightType.DIRECTIONAL, [-10, 12, -6], [0, 0, 0], [1.0, 0.95, 0.9], 0.5)

renderer.getRendererVariable("World.EnvIntensityScale").setFromFloatArray([0.05])

cam_transform = scene.get_main_camera().getTransform()
cam_transform.lookAt(vec3([0, 10, 24]), vec3([0, 2, 0]), vec3([0, 1, 0]))
