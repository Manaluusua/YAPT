
from yapt.resources import Resources
from py_yapt import Material, vec3

resources = yapt_instance.get_resources()
renderer = yapt_instance.get_renderer()
scene = yapt_instance.get_scene()

#envmap
env_map = resources.load_texture_2D("D:/Random/3DSampleAssets/EnvMapSources/photo_studio_loft_hall_4k.exr", "R16G16B16A16_SFLOAT")

if(env_map != None):
    renderer.getRendererVariable("World.Skycube").setTexture(env_map)

#meshes
mesh_array, mappings = resources.load_meshes_from_path("D:/Random/3DSampleAssets/Sphere/sphere.glb")[0]
sphere_mesh = mesh_array[0]

#enclosing light
createEnclosingSphereLight = True
if createEnclosingSphereLight == True:
	light = scene.create_render_object("enclosingLightObject")
	mat = renderer.createMaterial("lightMaterial")
	mat.setEmission(vec3([1, 1, 1]))
	mat.setTwoSided(True)
	light.setMesh(sphere_mesh)
	light.setMaterial(mat, 0)
	light.getTransform().setScale(vec3(1000));

#material spheres
scale = 3
numberOfColumns = 6
numberOfRows = 7
gap = 10
offset = [-gap * numberOfColumns * 0.5, -gap * numberOfRows * 0.5]

	 

for i in range(numberOfColumns):
	for j in range(numberOfRows):
	
		model = scene.create_render_object(f"object_{i}_{j}")

		val1 = float(i) / max(1, (numberOfColumns - 1))
		val2 = float(j) / max(1, (numberOfRows - 1))
		 
		mat = renderer.createMaterial(f"mat_{i}_{j}")

		if j == 0:
			mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
			mat.setSheenAmount(1)
			mat.setSpecularAmount(0)
			mat.setSheenRoughness(val1)
		elif j == 1:
			mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
		elif j == 2:
			mat.setFromMaterialPreset(Material.MaterialPreset.METAL_ALUMINIUM)
		elif j == 3:
			mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
			mat.setDielectricIOR(1.5)
		elif j == 4:
			mat.setFromMaterialPreset(Material.MaterialPreset.GLASS)
			mat.setDielectricIOR(1.5)
			mat.setEnableDispersion(True)
		elif j == 5:
			mat.setFromMaterialPreset(Material.MaterialPreset.PLASTIC)
			mat.setClearCoatAmount(1)
			mat.setClearCoatIOR(2.0)
		elif j == 6:
			mat.setFromMaterialPreset(Material.MaterialPreset.METAL_ALUMINIUM)
			mat.setClearCoatAmount(1)
		else:
			mat.setFromMaterialPreset(Material.MaterialPreset.METAL_COPPER)
		
		mat.setRoughness(val1)
		mat.setSpecularTint(vec3(1))
		mat.setAlbedo(vec3(1))

		pos = vec3([offset[0] + gap * i, offset[1] + gap * j, 0])

		model.setMesh(sphere_mesh)
		model.setMaterial(mat, 0)

		model.getTransform().setScale(vec3(scale))
		model.getTransform().setTranslation(pos)
	
scene.get_main_camera().getTransform().setTranslation(vec3([0, -5, 70]))
