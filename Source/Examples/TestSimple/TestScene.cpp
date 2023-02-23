#include "TestScene.h"
#include <Renderer/MeshUtility.h>
#include <Renderer/ResourceAllocationPool.h>
#include <Renderer/Mesh.h>
#include <Renderer/RenderObject.h>
#include <Common/CommonUtilities.h>
#include <Math/RandUtility.h>
#include <AssetLoader/AssetLoader.h>
#include <Scene/Camera.h>
#include <Common/CommonUtilities.h>

#define ASSETPATH "D:/Random/3DSampleAssets/"
#define CUBEMAPS_PATH "D:/Random/3DSampleAssets/EnvMaps/"

//#define ASSETPATH ""
//#define CUBEMAPS_PATH ""

#define ENVMAP_PATH CUBEMAPS_PATH "skybox_studio.ktx"
//#define MODEL_PATH "I:/Libraries/glTF-Sample-Models/2.0/Suzanne/glTF/Suzanne.gltf"
//#define MODEL_PATH "I:/Libraries/glTF-Sample-Models/2.0/BoomBox/glTF-Binary/BoomBox.glb"
#define TORUS_PATH ASSETPATH "Torus/torus.glb"
#define SPHERE_PATH ASSETPATH "Sphere/sphere.glb"
#define DRAGONXYZ_PATH ASSETPATH "XyzDragon/xyzDragon.glb"

#define BOWL_PATH ASSETPATH "Bowl/bowl.glb"
#define PLANE_PATH ASSETPATH "Plane/plane.glb"

using namespace YAPT;

TestScene::TestScene(YAPT::Renderer* renderer, YAPT::Gui* gui, YAPT::Scene* scene)
	:m_renderer(renderer),
	m_gui(gui),
	m_scene(scene)
{
	buildScene();
	m_scene->addSceneListener(this);
}

void TestScene::buildScene()
{
	loadMeshes();
	loadEnvMap();

	createWhiteFurnaceTestScene();
	//createMaterialComparisonScene();
	//createRoughnessComparison();
	//createCarPaintComparison();
	//createHeroShotScene();
}

void TestScene::createWhiteFurnaceTestScene()
{
	float scale = 3.f;
	size_t numberOfColumns = 6;
	size_t numberOfRows = 6;
	 
	const float gap = 10.f;

	vec2p offset = vec2p(-gap * numberOfColumns * 0.5f, -gap * numberOfRows * 0.5f);

	 

	for (size_t i = 0; i < numberOfColumns; ++i)
	{
		for (size_t j = 0; j < numberOfRows; ++j)
		{
			YAPT::Model* model = m_scene->createModel();
			m_models.push_back(model);

			float val1 = float(i) / std::max(size_t(1), (numberOfColumns - 1));

			float val2 = float(j) / std::max(size_t(1), (numberOfRows - 1));
			 
			YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
			mat->Release();
			switch (j)
			{
			case 0:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::PLASTIC);
				mat->setSheenAmount(1);
				mat->setSpecularAmount(0.f);
				mat->setSheenRoughness(val1);
				break;
			case 1:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::PLASTIC);
				break;
			case 2:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_ALUMINIUM);
				break;
			case 3:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::GLASS);
				mat->setDielectricIOR(1.5f);
				break;
			case 4:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::PLASTIC);
				mat->setClearCoatAmount(1.f);
				mat->setClearCoatIOR(2.0f);
				break;
			case 5:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_ALUMINIUM);
				mat->setClearCoatAmount(1.f);
				break;
			default:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_COPPER);
			}
			 
			mat->setRoughness(val1);

			mat->setSpecularTint(vec3p(1.f));
			mat->setAlbedo(vec3p(1.f));

			model->setMesh(m_sphereMesh);

			model->setMaterial(mat);
			model->Release();

			model->getTransform().setScale(vec3p(scale));

			vec3p pos(offset.x + gap * i, offset.y + gap * j, 0.f);

			model->getTransform().setTranslation(pos);
		}


	}


	m_scene->getMainCamera()->getTransform().setTranslation(vec3p(0.f, -5.f, 55.f));

}

void TestScene::createHeroShotScene()
{
	YAPT::Model* model = m_scene->createModel();
	m_models.push_back(model);
	YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
	mat->Release();

	mat->setFromMaterialPreset(YAPT::MaterialPreset::GLASS);
	mat->setRoughness(0.05f);

	float scale = 0.1f;

	model->setMesh(m_dragonMesh);
	model->setMaterial(mat);
	model->Release();

	mat->setAbsorption({ 0.4, 0.05, 0.2 });

	/*mat->setAnisotropy(0.95f);
	mat->setAnisotropyRotation(0.1f);
	mat->setClearCoatAmount(1.f);
	mat->setClearCoatIOR(1.7);
	mat->setClearCoatRoughness(0.09f);
	*/
	model->getTransform().setScale(vec3p(scale));
	vec3p pos(0.f, 0.f, -10.f);
	model->getTransform().setOrientation(glm::angleAxis(glm::pi<float>() * 0.5f, vec3p(1.f, 0.f, 0.f)));
	model->getTransform().setTranslation(pos);
}

void TestScene::createCarPaintComparison()
{
	float scale = 0.04f;
	

	size_t i = 0;
	size_t j = 0;

	std::pair<vec3p, vec3p> colors[] =
	{
		{{0.2f, 0.6f, 0.1f},{0.5f, 0.9f, 0.3f}},
		{{0.6f, 0.6f, 0.1f},{0.8f, 0.8f, 0.5f}},
		{{0.92549, 0.68627, 0.50196},{0.9960, 0.945098, 0.8196}},
		{{0.9451, 0.7294, 0.37255},{1.0, 0.97255, 0.73333}},
		{ {0.9607, 0.9490, 0.9176},{1.0, 1.0, 1.0}}
	};

	size_t numberOfColumns = YAPT::countOf(colors);
	size_t numberOfRows = 1;

	const float gap = 10.f;

	vec2p offset = vec2p(-gap * numberOfColumns * 0.5f, -gap * numberOfRows * 0.5f);
	for (size_t i = 0; i < numberOfColumns; ++i)
	{
		for (size_t j = 0; j < numberOfRows; ++j)
		{
			YAPT::Model* model = m_scene->createModel();
			m_models.push_back(model);
			YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
			mat->Release();

			mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_COPPER);
			mat->setRoughness(0.6);

			 
			mat->setAlbedo(colors[i].first);

			mat->setSpecularTint(colors[i].second);
			//mat->setSpecularTint(vec3p(1.f));
			//mat->setTwoSided(true);
			//mat->setAbsorption(vec3p(0.04f, 0.02f, 0.3f));
			//mat->setTransparency(transparency);
			//mat->setMetalFlakeDensity(0.0);
			mat->setAnisotropy(0.95f);
			mat->setAnisotropyRotation(0.1f);

			mat->setClearCoatAmount(1.f);
			mat->setClearCoatIOR(1.7);
			mat->setClearCoatRoughness(0.09f);mat->setRoughness(0.6);

			 
			mat->setAlbedo(colors[i].first);

			mat->setSpecularTint(colors[i].second);
			//mat->setSpecularTint(vec3p(1.f));
			//mat->setTwoSided(true);
			//mat->setAbsorption(vec3p(0.04f, 0.02f, 0.3f));
			//mat->setTransparency(transparency);
			//mat->setMetalFlakeDensity(0.0);
			mat->setAnisotropy(0.95f);
			mat->setAnisotropyRotation(0.1f);

			mat->setClearCoatAmount(1.f);
			mat->setClearCoatIOR(1.7);
			mat->setClearCoatRoughness(0.09f);
			model->setMesh(m_dragonMesh);
			model->setMaterial(mat);
			model->Release();

			model->getTransform().setScale(vec3p(scale));

			vec3p pos(offset.x + gap * i, offset.y + gap * j, 0.f);

			model->getTransform().setTranslation(pos);

			auto q = glm::angleAxis(glm::pi<float>() * 0.5f, vec3p(1.f, 0.f, 0.f));
			model->getTransform().setOrientation(q * glm::angleAxis(glm::pi<float>() * 0.15f, vec3p(0.f, 0.f, 1.f)));
		}

	}

	

	

	/*{
		YAPT::Model* model = m_scene->createModel();
		YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
		mat->Release();
		mat->setFromMaterialPreset(YAPT::MaterialPreset::PLASTIC);
		mat->setAlbedo(vec3p(0.1f));
		mat->setRoughness(0.2f);
		m_models.push_back(model);
		model->setMesh(m_sphereMesh);
		model->setMaterial(mat);
		model->Release();

		YAPT::vec3p pos(-12.f, -20.f, 0.f);

		model->getTransform().setTranslation(pos);
		model->getTransform().setScale(vec3p(15.f));

	}

	//lights
	{
		float emission = 6.f;
		YAPT::Model* model = m_scene->createModel();
		YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
		mat->Release();
		mat->setEmission(vec3p(emission, emission, emission));
		m_models.push_back(model);
		model->setMesh(m_torusMesh);
		model->setMaterial(mat);
		model->Release();

		YAPT::vec3p pos(0.f, 0.f, -15.f);

		model->getTransform().setTranslation(pos);
		model->getTransform().setScale(vec3p(6.f));
	}
	*/
	m_scene->getMainCamera()->getTransform().setTranslation(vec3p(0.f, -5.f, 55.f));
	//m_scene->getMainCamera()->getTransform().setOrientation(glm::angleAxis(glm::pi<float>(), vec3p(0.f, 1.f, 0.f)));
}

void TestScene::createRoughnessComparison()
{
	float scale = 0.3f;
	size_t numberOfColumns = 6;
	size_t numberOfRows = 6;

	const float gap = 10.f;

	vec2p offset = vec2p(-gap * numberOfColumns * 0.5f, -gap * numberOfRows * 0.5f);

	 

	for (size_t i = 0; i < numberOfColumns; ++i)
	{
		for (size_t j = 0; j < numberOfRows; ++j)
		{
			YAPT::Model* model = m_scene->createModel();
			m_models.push_back(model);

			float val1 = float(i) / std::max(size_t(1), (numberOfColumns - 1));

			float val2 = float(j) / std::max(size_t(1), (numberOfRows - 1));


			//val1 = 0.9;
			//val2 = 0.8f;
			  
			YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
			mat->Release();
			switch (j)
			{
			case 1:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_GOLD);
				break;
			case 2:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_BRASS);
				break;
			case 3:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_SILVER);
				break;
			case 4:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_ALUMINIUM);
				break;
			case 5:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::GLASS);
				mat->setAbsorption({ 0.8, 0.02, 0.2 });
				break;
			default:
				mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_COPPER);
			}
			//mat->setFromMaterialPreset(YAPT::MaterialPreset::METAL_COPPER);
			mat->setRoughness(0.1f + 0.8f*val1 );

			//mat->setAlbedo(vec3p(1.f));
			//mat->setAlbedo(vec3p(0.5f, 0.1f, 0.01f));

			//mat->setSpecularTint(vec3p(1.f));
			//mat->setTwoSided(true);
			//mat->setAbsorption(vec3p(0.04f, 0.02f, 0.3f));
			//mat->setTransparency(transparency);
			//mat->setMetalFlakeDensity(val2);
			mat->setAnisotropy(0.95f);
			mat->setAnisotropyRotation(0.12f);
			 
			//mat->setDielectricIOR(1.01f + val2 * 2.5f);

			//mat->setSpecularAmount(val2);
			//mat->setClearCoatAmount(1.f);
			//mat->setClearCoatIOR(1.6);
			//mat->setClearCoatRoughness(0.03f);
			model->setMesh(m_bowlMesh);
			model->setMaterial(mat);
			model->Release();

			model->getTransform().setScale(vec3p(scale));

			vec3p pos(offset.x + gap * i, offset.y + gap * j, 0.f);

			model->getTransform().setTranslation(pos);
		}
		
		
	}

	/*{
		YAPT::Model* model = m_scene->createModel();
		YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
		mat->Release();
		mat->setFromMaterialPreset(YAPT::MaterialPreset::PLASTIC);
		mat->setAlbedo(vec3p(0.1f));
		mat->setRoughness(0.2f);
		m_models.push_back(model);
		model->setMesh(m_sphereMesh);
		model->setMaterial(mat);
		model->Release();

		YAPT::vec3p pos(-12.f, -20.f, 0.f);

		model->getTransform().setTranslation(pos);
		model->getTransform().setScale(vec3p(15.f));

	}

	//lights
	{
		float emission = 6.f;
		YAPT::Model* model = m_scene->createModel();
		YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
		mat->Release();
		mat->setEmission(vec3p(emission, emission, emission));
		m_models.push_back(model);
		model->setMesh(m_torusMesh);
		model->setMaterial(mat);
		model->Release();
		 
		YAPT::vec3p pos(0.f, 0.f, -15.f);

		model->getTransform().setTranslation(pos);
		model->getTransform().setScale(vec3p(6.f));
	}
	*/
	m_scene->getMainCamera()->getTransform().setTranslation(vec3p(0.f, -5.f, 55.f));
	//m_scene->getMainCamera()->getTransform().setOrientation(glm::angleAxis(glm::pi<float>(), vec3p(0.f, 1.f, 0.f)));
}

void TestScene::createMaterialComparisonScene()
{
	float planeScale = 150.f;

	  
	YAPT::RCObjectPtr<YAPT::Material> defaultPlaneMat = m_renderer->createMaterial();
	defaultPlaneMat->Release();
	defaultPlaneMat->setFromMaterialPreset(YAPT::MaterialPreset::PLASTIC);
	defaultPlaneMat->setSpecularAmount(0.6);
	//defaultPlaneMat->setEmission(vec3p(0.01f));
	defaultPlaneMat->setRoughness(0.3);
	defaultPlaneMat->setAlbedo(vec3p(0.001, 0.001, 0.001));
	defaultPlaneMat->setDielectricIOR(1.3);
	defaultPlaneMat->setTwoSided(true);
	//bottom
	{
		YAPT::Model* model = m_scene->createModel();
		m_models.push_back(model);

		YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
		mat->Release();
		mat->setFromMaterialPreset(YAPT::MaterialPreset::PLASTIC);
		mat->setSpecularAmount(0.0f);
		mat->setAlbedo(vec3p(0.1f, 0.1f, 0.1f));
		mat->setClearCoatAmount(0.f);
		mat->setRoughness(0.7f);
		mat->setTwoSided(true);
		
		model->setMesh(m_planeMesh);
		model->setMaterial(mat);
		model->getTransform().setScale(vec3p(planeScale));
		model->getTransform().addTranslation(vec3p(0.f, -planeScale, 0.f));
		model->Release();
	}
	
	//back
	/*{
		YAPT::Model* model = m_scene->createModel();
		m_models.push_back(model);
		model->setMesh(m_planeMesh);
		model->setMaterial(defaultPlaneMat);
		model->getTransform().setScale(vec3p(planeScale));
		glm::quat q = glm::angleAxis(0.5f * glm::pi<float>(), vec3p(1.f, 0.0f, 0.0f));
		model->getTransform().setOrientation(q);
		model->getTransform().addTranslation(vec3p(0.f, 0.f, -planeScale));
		model->Release();
	}
	
	//front
	{
		YAPT::Model* model = m_scene->createModel();
		m_models.push_back(model);
		model->setMesh(m_planeMesh);
		model->setMaterial(defaultPlaneMat);
		model->getTransform().setScale(vec3p(planeScale));
		glm::quat q = glm::angleAxis(-0.5f * glm::pi<float>(), vec3p(1.f, 0.0f, 0.0f));
		model->getTransform().setOrientation(q);
		model->getTransform().addTranslation(vec3p(0.f, 0.f, planeScale));
		model->Release();
	}
	//left
	{
		YAPT::Model* model = m_scene->createModel();
		m_models.push_back(model);
		model->setMesh(m_planeMesh);
		model->setMaterial(defaultPlaneMat);
		model->getTransform().setScale(vec3p(planeScale));
		glm::quat q = glm::angleAxis(0.5f * glm::pi<float>(), vec3p(0.f, 0.0f, 1.0f));
		model->getTransform().setOrientation(q);
		model->getTransform().addTranslation(vec3p(-planeScale, 0.f, 0.f));
		model->Release();
	}*/
	/*

	//right
	{
		YAPT::Model* model = m_scene->createModel();
		m_models.push_back(model);
		model->setMesh(m_planeMesh);
		model->setMaterial(defaultPlaneMat);
		model->getTransform().setScale(vec3p(planeScale));
		glm::quat q = glm::angleAxis(0.5f * glm::pi<float>(), vec3p(0.f, 0.0f, 1.0f));
		model->getTransform().setOrientation(q);
		model->getTransform().addTranslation(vec3p(planeScale, 0.f, 0.f));
		model->Release();
	}

	*/
	size_t sphereCount = 5;
	float sphereGap = 20.f;
	for(size_t i = 0; i < sphereCount; ++i)
	{
		float v = float(i) / (sphereCount - 1);
		YAPT::Model* model = m_scene->createModel();
		YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
		mat->Release();
		mat->setFromMaterialPreset(YAPT::MaterialPreset::GLASS);

		mat->setDielectricIOR(1.5f);
		mat->setRoughness(0.02f + v * 0.6f);
		mat->setAbsorption(vec3p(0.03f, 0.01f, 0.002f) * 0.4f);
		 
		m_models.push_back(model);
		model->setMesh(m_bowlMesh);
		model->setMaterial(mat);
		model->Release();

		YAPT::vec3p pos(-sphereGap * sphereCount * 0.5f + i * sphereGap, -planeScale + 15.f, 0.f);

		model->getTransform().setTranslation(pos);
		model->getTransform().setScale(vec3p(0.3f));

	}
	//light
	for (size_t i = 0; i < sphereCount; ++i)
	{
		if (i == 1)
		{
			float emission = 20;
			YAPT::Model* model = m_scene->createModel();
			YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
			mat->Release();
			mat->setEmission(vec3p(emission));
			m_models.push_back(model);
			model->setMesh(m_sphereMesh);
			model->setMaterial(mat);
			model->Release();

			YAPT::vec3p pos(-sphereGap * sphereCount * 0.5f + i * sphereGap, -planeScale + 30.f, 0.f);
			model->getTransform().setTranslation(pos);
			model->getTransform().setScale(vec3p(1.f));
		}
		else if(i >= 3)
		{
			float emission = 10;
			YAPT::Model* model = m_scene->createModel();
			YAPT::RCObjectPtr<YAPT::Material> mat = m_renderer->createMaterial();
			mat->Release();
			mat->setEmission(vec3p(emission));
			m_models.push_back(model);
			model->setMesh(m_torusMesh);
			model->setMaterial(mat);
			model->Release();

			YAPT::vec3p pos(-sphereGap * sphereCount * 0.5f + i * sphereGap, -planeScale + 15.f, 0.f);

			model->getTransform().setTranslation(pos);
			model->getTransform().setScale(vec3p(1.f));
		}
		
	}

	{
		

	}
	
	m_scene->getMainCamera()->getTransform().setTranslation(vec3p(0.f, -planeScale + 20.f, 50.f));

}

void TestScene::loadMeshes()
{
	m_sphereMesh = loadMesh(SPHERE_PATH);
	m_torusMesh = loadMesh(TORUS_PATH);
	//m_bowlMesh = loadMesh(BOWL_PATH);
	m_planeMesh = loadMesh(PLANE_PATH);
	//m_dragonMesh = loadMesh(DRAGONXYZ_PATH);
}



void TestScene::loadEnvMap()
{
	//load skybox
	{
		YAPT::TextureLoadingContext* texLoader = YAPT::getAssetLoader()->loadTexture(ENVMAP_PATH);
		const YAPT::TextureLoadInfo& info = texLoader->getTextureInfo();

		YAPT::ResourceAllocationPool* resChunk = m_renderer->createResourceAllocationPool();
		uint32_t w = texLoader->getSubTextureInfo(0, 0).width;
		uint32_t h = texLoader->getSubTextureInfo(0, 0).height;

		m_skyCube = resChunk->addTexture("skycube", info.dimension, info.format, YAPT::ResourceUsageBits::RESOURCE_USAGE_SAMPLED_TEXTURE | YAPT::ResourceUsageBits::RESOURCE_USAGE_COPY_DESTINATION,
			w, h, info.numberOfMips, info.numberOfSlices);

		m_skyCube->Release();
		resChunk->allocateAndConsume();

		//upload
		std::vector<YAPT::TextureDataDefinition> uploadData;
		for (uint32_t s = 0; s < info.numberOfSlices; ++s)
		{
			for (uint32_t m = 0; m < info.numberOfMips; ++m)
			{
				YAPT::TextureDataDefinition d;
				d.rowPitchInBytes = texLoader->getSubTextureInfo(m, s).rowPitch;
				d.data = texLoader->getData(m, s);
				uploadData.push_back(d);
			}
		}

		m_skyCube->upload(0, 0, info.numberOfMips, info.numberOfSlices, uploadData.data());

		m_renderer->getRendererConfiguration()->getRendererVariable("World.Skycube")->set(m_skyCube);

	}
}
YAPT::RCObjectPtr<YAPT::Mesh> TestScene::loadMesh(const char* path)
{ 
	YAPT::RCObjectPtr<YAPT::Mesh> mesh;
	{
		YAPT::SceneLoadingContext* modelLoader = YAPT::getAssetLoader()->loadScene(path);
		YAPT::ResourceAllocationPool* resChunk = m_renderer->createResourceAllocationPool();

		std::vector< YAPT::RCObjectPtr<YAPT::Buffer>> vertexBuffers;
		YAPT::RCObjectPtr<YAPT::Buffer> indexBuffer;

		assert(modelLoader->getNumberOfMeshes() > 0);

		const YAPT::MeshLoadInfo& info = modelLoader->getMeshLoadInfo(0);

		vertexBuffers.resize(info.numberOfVertexBuffers);
		//create buffers
		for (size_t i = 0; i < info.numberOfVertexBuffers; ++i)
		{
			//const YAPT::VertexBufferLayout& layout = info.layouts[i];
			size_t bufferIndex = info.vertexBufferIndices[i];
			const YAPT::BufferLoadInfo& buffInfo = modelLoader->getBufferLoadInfo(bufferIndex);

			vertexBuffers[i] = resChunk->addBuffer("Model VertexBuffer", YAPT::RESOURCE_USAGE_VERTEX_BUFFER | YAPT::RESOURCE_USAGE_COPY_DESTINATION, buffInfo.sizeInBytes);
			vertexBuffers[i]->Release();
		}
		{
			const YAPT::BufferLoadInfo& buffInfo = modelLoader->getBufferLoadInfo(info.indexBufferIndex);
			indexBuffer = resChunk->addBuffer("Model IndexBuffer", YAPT::RESOURCE_USAGE_INDEX_BUFFER | YAPT::RESOURCE_USAGE_COPY_DESTINATION, buffInfo.sizeInBytes);
			indexBuffer->Release();
		}
		
		resChunk->allocateAndConsume();

		//upload buffers
		for (size_t i = 0; i < info.numberOfVertexBuffers; ++i)
		{
			size_t bufferIndex = info.vertexBufferIndices[i];
			const YAPT::BufferLoadInfo& buffInfo = modelLoader->getBufferLoadInfo(bufferIndex);
			vertexBuffers[i]->upload(0, buffInfo.sizeInBytes, modelLoader->getBufferData(bufferIndex));
			
		}
		{
			const YAPT::BufferLoadInfo& buffInfo = modelLoader->getBufferLoadInfo(info.indexBufferIndex);
			indexBuffer->upload(0, buffInfo.sizeInBytes, modelLoader->getBufferData(info.indexBufferIndex));
		}

		//create mesh
		mesh = m_renderer->createMesh(info.layouts, info.numberOfVertexBuffers, info.vertexCount);
		for (size_t i = 0; i < info.numberOfVertexBuffers; ++i)
		{
			mesh->setVertexBuffer(i, vertexBuffers[i].get(), 0);
		}
		
		mesh->setIndexBuffer(indexBuffer.get(), 0, info.primitiveCount);
		mesh->Release();
		
	}
	return mesh;
}



TestScene::~TestScene()
{
	m_scene->removeSceneListener(this);
}

void TestScene::beforeUpdate(const YAPT::SceneUpdateParameters& update)
{
	/*double time = update.overallElapsedTimeInSeconds;

	YAPT::quat q = glm::angleAxis(float(time) * glm::pi<float>() * 0.5f, vec3p(1.0, 0.0, 0.0));
	
	for (size_t i = 0; i < m_models.size(); ++i)
	{
		m_models[i]->getTransform().setOrientation(q);
	}*/
}
 

