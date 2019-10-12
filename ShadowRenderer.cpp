#include "stdafx.h"
#include "ShadowMapRenderer.h"
#include "ContentManager.h"
#include "ShadowMapMaterial.h"
#include "RenderTarget.h"
#include "MeshFilter.h"
#include "SceneManager.h"
#include "OverlordGame.h"


ShadowMapRenderer::~ShadowMapRenderer()
{
	delete m_pShadowMat;
	delete m_pShadowRT;
}

void ShadowMapRenderer::Initialize(const GameContext& gameContext)
{
	if (m_IsInitialized)
		return;

	UNREFERENCED_PARAMETER(gameContext);
	//create shadow generator material + initialize it
	//create a rendertarget with the correct settings (depth only) for the shadow generator using a RENDERTARGET_DESC

	m_pShadowMat = new ShadowMapMaterial();
	m_pShadowMat->Initialize(gameContext);

	auto gameSettings = OverlordGame::GetGameSettings();

	RENDERTARGET_DESC desc{};
	desc.Width = gameSettings.Window.Width;
	desc.Height = gameSettings.Window.Height;
	desc.EnableColorBuffer = false;
	desc.EnableDepthSRV = true;
	desc.EnableDepthBuffer = true;
	desc.EnableColorSRV = false;

	m_pShadowRT = new RenderTarget(gameContext.pDevice);
	auto hr = m_pShadowRT->Create(desc);
	if(!FAILED(hr))
		m_IsInitialized = true;
}

void ShadowMapRenderer::SetLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 direction)
{
	
	//calculate the Light VP matrix and store it in the appropriate datamember

	m_LightPosition = position;
	m_LightDirection = direction;

	auto gameSettings = OverlordGame::GetGameSettings();

	DirectX::XMVECTOR lightPositionVec = DirectX::XMLoadFloat3(&m_LightPosition);
	DirectX::XMVECTOR lightDirectionVec = DirectX::XMLoadFloat3(&m_LightDirection);
	DirectX::XMVECTOR upVec = { 0,1,0 };

	//Alternative: XMMatrixLookAtLH
	DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(lightPositionVec, lightDirectionVec, upVec);
	float viewWidth = (m_Size > 0) ? m_Size * gameSettings.Window.AspectRatio : gameSettings.Window.Width;
	float viewHeight = (m_Size > 0) ? m_Size : gameSettings.Window.Height;

	DirectX::XMMATRIX projection = DirectX::XMMatrixOrthographicLH(viewWidth, viewHeight, 0.1f, 2500.f);
	DirectX::XMMATRIX vp = view * projection;
	XMStoreFloat4x4(&m_LightVP, vp);
}

void ShadowMapRenderer::Begin(const GameContext& gameContext) const
{
	//Reset Texture Register 5 (Unbind)
	ID3D11ShaderResourceView *const pSRV[] = { nullptr };
	gameContext.pDeviceContext->PSSetShaderResources(1, 1, pSRV);

	UNREFERENCED_PARAMETER(gameContext);
	//set the appropriate render target that our shadow generator will write to 
	//clear this render target
	SceneManager::GetInstance()->GetGame()->SetRenderTarget(m_pShadowRT);
	m_pShadowRT->Clear(gameContext, DirectX::Colors::CornflowerBlue);
	m_pShadowMat->SetLightVP(m_LightVP);

	
}

void ShadowMapRenderer::End(const GameContext& gameContext) const
{
	UNREFERENCED_PARAMETER(gameContext);
	

	SceneManager::GetInstance()->GetGame()->SetRenderTarget(nullptr);

	RenderTarget* rt = SceneManager::GetInstance()->GetGame()->GetRenderTarget();
	rt->Clear(gameContext, DirectX::Colors::CornflowerBlue);
}

void ShadowMapRenderer::Draw(const GameContext& gameContext, MeshFilter* pMeshFilter, DirectX::XMFLOAT4X4 world, const std::vector<DirectX::XMFLOAT4X4>& bones) const
{
	ID3DX11EffectTechnique* tech = nullptr;
	VertexBufferData vertBufferData;
	
	m_pShadowMat->SetWorld(world);
	m_pShadowMat->SetLightVP(m_LightVP);
	
	

	if (pMeshFilter->m_HasAnimations)
	{
		tech = m_pShadowMat->m_pShadowTechs[1];
		gameContext.pDeviceContext->IASetInputLayout(m_pShadowMat->m_pInputLayouts[1]);
		vertBufferData = pMeshFilter->GetVertexBufferData(gameContext, m_pShadowMat->m_InputLayoutIds[1]);
		m_pShadowMat->SetBones((&bones[0]._11),bones.size());
	}
	else
	{
		tech = m_pShadowMat->m_pShadowTechs[0];
		gameContext.pDeviceContext->IASetInputLayout(m_pShadowMat->m_pInputLayouts[0]);
		vertBufferData = pMeshFilter->GetVertexBufferData(gameContext, m_pShadowMat->m_InputLayoutIds[0]);
	}

	
	//Set Vertex Buffer
	UINT offset = 0;
	gameContext.pDeviceContext->IASetVertexBuffers(0, 1, &vertBufferData.pVertexBuffer, &vertBufferData.VertexStride, &offset);

	//Set Index Buffer
	gameContext.pDeviceContext->IASetIndexBuffer(pMeshFilter->m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	//Set Primitive Topology
	gameContext.pDeviceContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	//DRAW
	
	D3DX11_TECHNIQUE_DESC techDesc;
	tech->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		tech->GetPassByIndex(p)->Apply(0, gameContext.pDeviceContext);
		gameContext.pDeviceContext->DrawIndexed(pMeshFilter->m_IndexCount, 0, 0);
	}
	
}

void ShadowMapRenderer::UpdateMeshFilter(const GameContext& gameContext, MeshFilter* pMeshFilter)
{
	UNREFERENCED_PARAMETER(gameContext);
	UNREFERENCED_PARAMETER(pMeshFilter);


	ShadowMapMaterial::ShadowGenType shadowType = (pMeshFilter->m_HasAnimations) ? ShadowMapMaterial::ShadowGenType::Skinned : ShadowMapMaterial::ShadowGenType::Static;
	pMeshFilter->BuildVertexBuffer(gameContext, m_pShadowMat->m_InputLayoutIds[shadowType], m_pShadowMat->m_InputLayoutSizes[shadowType], m_pShadowMat->m_InputLayoutDescriptions[shadowType]);
}

ID3D11ShaderResourceView* ShadowMapRenderer::GetShadowMap() const
{
	
	return m_pShadowRT->GetDepthShaderResourceView();
}
