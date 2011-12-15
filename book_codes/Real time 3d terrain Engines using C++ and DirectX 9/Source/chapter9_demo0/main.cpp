
#include "gaia\gaia.h"
#include "resource.h"


using namespace gaia;

			
class cMyHost:public cGameHost
{
public:
	cMyHost(){};
	~cMyHost(){};

	HRESULT InitDeviceObjects();
	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();
	HRESULT DeleteDeviceObjects();
	HRESULT updateScene();
	HRESULT renderScene();
	HRESULT OneTimeSceneInit(); 

    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	cTerrain m_terrainSystem;
	cTexture* m_pHeightMap;
	cTexture* m_pBlendMap;
	cTexture* m_pGrass;
	cTexture* m_pRock;
	cTexture* m_pDirt;

	cRenderMethod* m_pRenderMethod;
	cSurfaceMaterial* m_pSurfaceMaterial;

};

HRESULT cMyHost::OneTimeSceneInit() 
{
	setupWorldQuadTree(
		cRect3d(-500.0f,
				500.0f,
				-500.0f,
				500.0f,
				0.0f,
				500.0f));
	return cGameHost::OneTimeSceneInit();
}

HRESULT cMyHost::InitDeviceObjects()
{
	cGameHost::InitDeviceObjects();

	// generate a random height map
	m_pHeightMap = 
		displayManager()
		.texturePool()
		.createResource(cString("height map"));
	m_pHeightMap->createTexture(
		128, 128, 
		1, 0, 
		D3DFMT_A8R8G8B8, 
		D3DPOOL_MANAGED);
	m_pHeightMap->generatePerlinNoise(
		0.01f, 5, 0.6f);

	// create a terrain from this map
	m_terrainSystem.create(
		&rootNode(), 
		m_pHeightMap, 
		worldExtents(), 
		3);

	// load our render method
	m_pRenderMethod = 
		TheGameHost
		.displayManager()
		.renderMethodPool()
		.createResource("terrain method");
	m_pRenderMethod->loadEffect(
		cRenderMethod::k_defaultMethod,
		"media\\shaders\\simple_terrain.fx");

	// generate three elevation structures
    cTerrain::elevationData elevation[3];

	// grass (all elevations and slopes)
	elevation[0].minElevation = 0;
	elevation[0].maxElevation = 500;
	elevation[0].minNormalZ = -1.0f;
	elevation[0].maxNormalZ = 1.0f;
	elevation[0].strength = 1.0f;

	// rock (all elevations, steep slopes)
	elevation[1].minElevation = 0;
	elevation[1].maxElevation = 500;
	elevation[1].minNormalZ = 0.0f;
	elevation[1].maxNormalZ = 0.85f;
	elevation[1].strength = 10.0f;

	// dirt (high elevation, flat slope)
	elevation[2].minElevation = 300;
	elevation[2].maxElevation = 500;
	elevation[2].minNormalZ = 0.75f;
	elevation[2].maxNormalZ = 1.0f;
	elevation[2].strength = 20.0f;

	// generate the blend image
	cImage* pBlendImage;
	pBlendImage = 
		displayManager()
		.imagePool()
		.createResource(cString("image map 3"));
	pBlendImage->create(
		256, 
		256, 
		cImage::k_32bit);

	m_terrainSystem.generateBlendImage(
		pBlendImage, 
		elevation, 3);

	pBlendImage->randomChannelNoise(3, 200, 255);

	// upload the blend image to a texture
	m_pBlendMap = 
		displayManager()
		.texturePool()
		.createResource(cString("image map"));
	
	m_pBlendMap->createTexture(
		256, 256, 
		1, 0, 
		D3DFMT_A8R8G8B8, 
		D3DPOOL_MANAGED);

	m_pBlendMap->uploadImage(pBlendImage);
	safe_release(pBlendImage);

	// load the ground surface textures
	m_pGrass = 
		displayManager()
		.texturePool()
		.createResource(cString("grass"));
	m_pRock = 
		displayManager()
		.texturePool()
		.createResource(cString("rock"));
	m_pDirt = 
		displayManager()
		.texturePool()
		.createResource(cString("dirt"));

	m_pGrass->loadResource(
		"media\\textures\\grass.dds");
	m_pRock->loadResource(
		"media\\textures\\rock.dds");
	m_pDirt->loadResource(
		"media\\textures\\dirt.dds");

	// create a surface material
	// and load our textures into it
	m_pSurfaceMaterial = 
		displayManager()
		.surfaceMaterialPool()
		.createResource(cString("ground material"));
	m_pSurfaceMaterial->setTexture(0, m_pBlendMap);
	m_pSurfaceMaterial->setTexture(1, m_pGrass);
	m_pSurfaceMaterial->setTexture(2, m_pRock);
	m_pSurfaceMaterial->setTexture(3, m_pDirt);

	// give the render method and material to the terrain
	m_pRenderMethod->setMaterial(0, m_pSurfaceMaterial);
	m_terrainSystem.setRenderMethod(m_pRenderMethod);

	// force-update the camera to make sure
	// we start above the terrain
	defaultCamera().update();
	updateCamera(
		10.0f, 
		0.1f, 
		&m_terrainSystem,
		30.0f, 
		true); //<- true forces an update

	return S_OK;
}

HRESULT cMyHost::RestoreDeviceObjects()
{
	cGameHost::RestoreDeviceObjects();

	return S_OK;
}

HRESULT cMyHost::InvalidateDeviceObjects()
{
	cGameHost::InvalidateDeviceObjects();

	return S_OK;
}

HRESULT cMyHost::DeleteDeviceObjects()
{
	cGameHost::DeleteDeviceObjects();

	m_terrainSystem.destroy();

	safe_release(m_pHeightMap);
	safe_release(m_pBlendMap);
	safe_release(m_pRenderMethod);
	safe_release(m_pSurfaceMaterial);

	safe_release(m_pGrass);
	safe_release(m_pRock);
	safe_release(m_pDirt);

	return S_OK;
}


HRESULT cMyHost::updateScene()
{
	profile_scope(scene_update);
	
	// call the basic camera UI 
	// function of cGameHost
	updateCamera(
		10.0f, 
		0.1f, 
		&m_terrainSystem,
		30.0f, 
		false);

	// call the UI function of the terrain
	m_terrainSystem.readUserInput();

	return cGameHost::updateScene();
}

HRESULT cMyHost::renderScene()
{
	profile_scope(scene_render);

	cSceneObject* pFirstMember = 
		quadTree().buildSearchResults(activeCamera()->searchRect());

	cSceneObject* pRenderList = pFirstMember;

	// prepare all objects for rendering
	while(pFirstMember)
	{
		pFirstMember->prepareForRender();
		pFirstMember = pFirstMember->nextSearchLink();
	}

	// render all objects (puts them in the render queue)
	pFirstMember = pRenderList;
	while(pFirstMember)
	{
		pFirstMember->render();
		pFirstMember = pFirstMember->nextSearchLink();
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message proc function to handle key and menu input
//-----------------------------------------------------------------------------
LRESULT cMyHost::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{
    // Pass remaining messages to default handler
    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}



//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain (HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	cMyHost d3dApp;

	srand(timeGetTime());

	//  InitCommonControls();
	if (FAILED (d3dApp.Create (hInst)))
	 return 0;

	INT result= d3dApp.Run();

	cCodeTimer::RootTimer.outputAllTimers(0xffffffff);

	return result;
}

