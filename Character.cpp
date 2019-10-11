
#include "stdafx.h"
#include "GameCharacter.h"
#include "Components.h"
#include "Prefabs.h"
#include "GameScene.h"
#include "PhysxManager.h"
#include "PhysxProxy.h"
#include "GameObject.h"
#include "DebugRenderer.h"
#include "ModelAnimator.h"
#include "../OverlordEngine/MathHelper.h"
#include "../OverlordProject/Materials/ColorMaterial.h"
#include "../OverlordProject/Materials/Shadow/SkinnedDiffuseMaterial_Shadow.h"
#include "GameprojectScene.h"
#include "SpriteComponent.h"
#include "OverlordGame.h"
#include "TextureData.h"
#include "ContentManager.h"
#include "SpriteFont.h"
#include "TextRenderer.h"
#include "SpriteFont.h"
#include "Blood.h"
#include "SoundManager.h"
#include "MovingPlatform.h"



using namespace DirectX;


GameCharacter::GameCharacter(float xPos, float yPos, float zPos,float radius, float height, float moveSpeed) :
	m_Radius(radius),
	m_Height(height),
	m_MoveSpeed(moveSpeed),
	m_pController(nullptr),
	m_TotalPitch(0), 
	m_TotalYaw(0),
	m_IsInit(false),
	m_Up(0,1,0),
	m_NewUp(0,1,0),
	m_IsGrounded(false),
	m_MoveDirection(0,0,0),
	m_Gravity(40.0f),
	m_JumpSpeed(20.0f),
	m_DownRaycastLength(15.0f),
	m_xOffset(xPos),
	m_yOffset(yPos),
	m_zOffset(zPos)
{}

void GameCharacter::Initialize(const GameContext& gameContext)
{
	UNREFERENCED_PARAMETER(gameContext);
	auto physX = PhysxManager::GetInstance()->GetPhysics();

	auto noBouncy = physX->createMaterial(1.f, 1.f, 0.f);
	//TODO: Create controller
	m_pController = new ControllerComponent(noBouncy, m_Radius, m_Height, L"Player", physx::PxCapsuleClimbingMode::eEASY);
	//m_pController->SetCollisionGroup(CollisionGroupFlag::Group0);
	AddComponent(m_pController);
	
	m_pController->SetCollisionIgnoreGroups(CollisionGroupFlag::Group7);

	m_pPlayerModelHolder = new GameObject();
	auto playerModel = new ModelComponent(L"Resources/Meshes/Knight_Game_2.ovm");
	m_pPlayerModelHolder->AddComponent(playerModel);
	auto pDiffMaterialPlayer = new SkinnedDiffuseMaterial_Shadow();
	pDiffMaterialPlayer->SetDiffuseTexture(L"Resources/Textures/Knight_Game_Color.png");
	pDiffMaterialPlayer->SetLightDirection(gameContext.pShadowMapper->GetLightDirection());
	gameContext.pMaterialManager->AddMaterial(pDiffMaterialPlayer, 7);
	playerModel->SetMaterial(7);
	
	AddChild(m_pPlayerModelHolder);
	m_pPlayerModelHolder->GetTransform()->Scale(0.03f, 0.03f, 0.03f);
	m_pPlayerModelHolder->GetTransform()->Translate(0.0f, -m_Height/2.0f - 2.0f, 0.0f);
	m_pPlayerModelHolder->GetTransform()->Rotate(0.0f, 180.0f, 0.0f);
	m_LastAngle = 180.0f;
	
	m_pCameraObject = new GameObject();
	auto cam = new CameraComponent();
	m_pCameraObject->AddComponent(cam);
	m_pCameraObject->GetTransform()->Translate(0.0f, 20.0f, -30.0f);
	m_pCameraObject->GetTransform()->Rotate(35.0f, 0.0f, 0.0f);
	
	AddChild(m_pCameraObject);
	//UI

	m_pFont = ContentManager::Load<SpriteFont>(L"./Resources/SpriteFonts/GillSans_64_nr.fnt");

	//Load in all health textures
	for (int i = 0; i <= m_MaxHealth; ++i)
	{
		TextureData* healthTexture = ContentManager::Load<TextureData>(m_HealthTexturePath + std::to_wstring(i) + L".png");
		m_pHealthTextures.push_back(healthTexture);
	}
	

	GameSettings gameSettings = OverlordGame::GetGameSettings();
	float xPos = gameSettings.Window.Width - 200.0f;
	float yPos = gameSettings.Window.Height - 600.0f;
	m_pHealthContainer = new GameObject();
	
	m_pHealthSprite = new SpriteComponent(*m_pHealthTextures[3] , DirectX::XMFLOAT2(0.5f, 0.5f), DirectX::XMFLOAT4(1, 1, 1, 1.0f));
	m_pHealthContainer->AddComponent(m_pHealthSprite);
	AddChild(m_pHealthContainer);
	

	m_pHealthContainer->GetTransform()->Translate(xPos, yPos, .99f);
	m_pHealthContainer->GetTransform()->Scale(0.55f, 0.55f, 1.0f);

	//star ui

	m_pStarCoinContainer = new GameObject();

	m_pStarCoinSprite = new SpriteComponent(L"Resources/Textures/staircoinUI.png", DirectX::XMFLOAT2(0.5f, 0.5f), DirectX::XMFLOAT4(1, 1, 1, 1.0f));
	m_pStarCoinContainer->AddComponent(m_pStarCoinSprite);
	AddChild(m_pStarCoinContainer);

	m_pStarCoinContainer->GetTransform()->Translate(120.0f, 100.0f, .99f);
	m_pStarCoinContainer->GetTransform()->Scale(0.25f, 0.25f, 1.0f);


	//set start hp
	m_CurrentHealth = m_MaxHealth;
	

	//TODO: Register all Input Actions
	gameContext.pInput->AddInputAction(InputAction(LEFT, Down, 'A'));
	gameContext.pInput->AddInputAction(InputAction(RIGHT, Down, 'D'));
	gameContext.pInput->AddInputAction(InputAction(FORWARD, Down, 'W'));
	gameContext.pInput->AddInputAction(InputAction(BACKWARD, Down, 'S'));
	gameContext.pInput->AddInputAction(InputAction(JUMP, Down, VK_SPACE,-1,XINPUT_GAMEPAD_A));
	cam->SetActive();
	m_Invulnerable = true;

	//run particles
	m_pEmitter = new GameObject();
	m_pParticleEmitter = new ParticleEmitterComponent(L"./Resources/Textures/RunParticle.png", 16);
	m_pParticleEmitter->SetVelocity(DirectX::XMFLOAT3(0, 0.5f, 0));
	m_pParticleEmitter->SetMinSize(1.0f);
	m_pParticleEmitter->SetMaxSize(2.0f);
	m_pParticleEmitter->SetMinEnergy(0.1f);
	m_pParticleEmitter->SetMaxEnergy(0.5f);
	m_pParticleEmitter->SetMinSizeGrow(0.5f);
	m_pParticleEmitter->SetMaxSizeGrow(-2.5f);
	m_pParticleEmitter->SetMinEmitterRange(0.2f);
	m_pParticleEmitter->SetMaxEmitterRange(.8f);
	m_pParticleEmitter->SetColor(DirectX::XMFLOAT4(1.f, 1.f, 1.f, 0.4f));
	m_pEmitter->AddComponent(m_pParticleEmitter);
	AddChild(m_pEmitter);
	m_pEmitter->GetTransform()->Translate(0.0f, -3.0f, 0.0f);
	m_pEmitter->SetEnabled(false);
	//m_pController->SetCollisionIgnoreGroups(CollisionGroupFlag::Group9);

	m_pEmitter->SetEnabled(false);

	SoundManager::GetInstance()->GetSystem()->createSound("Resources/Sounds/jump.wav", FMOD_2D , NULL, &m_pJumpSound);
	SoundManager::GetInstance()->GetSystem()->createSound("Resources/Sounds/hurt.wav", FMOD_2D, NULL, &m_pHurtSound);
	
	GetTransform()->Translate(m_xOffset, m_yOffset, m_zOffset);

	m_IsInit = true;
}

void GameCharacter::PostInitialize(const GameContext& gameContext)
{
	UNREFERENCED_PARAMETER(gameContext);
	m_pAnimator = m_pPlayerModelHolder->GetComponent<ModelComponent>()->GetAnimator();
	if (m_pAnimator) 
	{
		m_pAnimator->SetAnimation(0);
		m_pAnimator->Play();
	}
		
}

void GameCharacter::UpdateGrounded()
{
	physx::PxRaycastBuffer hitInfo;
	physx::PxVec3 start = ToPxVec3(m_pController->GetFootPosition());
	physx::PxVec3 dir = ToPxVec3(m_pController->GetUp());
	physx::PxQueryFilterData filter;
	filter.data.word0 = CollisionGroupFlag::Group4;
	m_IsGrounded = GetScene()->GetPhysxProxy()->Raycast(start, -dir, 0.02f, hitInfo,physx::PxHitFlag::eDEFAULT,filter);
}

void GameCharacter::UpdateUp()
{
	physx::PxRaycastBuffer hitInfo;
	physx::PxVec3 start = ToPxVec3(m_pController->GetFootPosition());
	physx::PxVec3 dir = ToPxVec3(m_pController->GetUp());
	physx::PxQueryFilterData filter;
	filter.data.word0 = CollisionGroupFlag::Group4;
//	filter.data.word1 = CollisionGroupFlag::Group6;

	auto physxProxy = GetScene()->GetPhysxProxy();
	bool hit = physxProxy->Raycast(start, -dir, m_DownRaycastLength, hitInfo, physx::PxHitFlag::eDEFAULT, filter);
	if (hit)
		if(hitInfo.block.normal != m_NewUp)
			m_NewUp = hitInfo.block.normal;
}

bool GameCharacter::IsHittingLava()
{
	physx::PxRaycastBuffer hitInfo;
	physx::PxVec3 start = ToPxVec3(m_pController->GetFootPosition());
	physx::PxVec3 dir = ToPxVec3(m_pController->GetUp());
	physx::PxQueryFilterData filter;
	filter.data.word0 = Group4;
	
	bool hit = GetScene()->GetPhysxProxy()->Raycast(start, -dir, 0.01f, hitInfo, physx::PxHitFlag::eDEFAULT, filter);
	if (hit)
	{
		auto const component = hitInfo.block.actor->userData;

		if (component == nullptr)
		{
			Logger::LogError(L"component is a nullpointer");
			return false;
		}
		GameObject* gameObject = reinterpret_cast<BaseComponent*>(component)->GetGameObject();
		
		return gameObject->GetTag() == L"Lava";
	}
	return false;
}



void GameCharacter::Update(const GameContext& gameContext)
{
	using namespace DirectX;

	// If not done with initializing then just return
	GameprojectScene* scene = static_cast<GameprojectScene*>(GetScene());
	
	if (!m_IsInit || !scene) return;


	

	//Update invulnerability if needed
	if (m_Invulnerable)
	{
		UpdateInvulnerability(gameContext.pGameTime->GetElapsed());
		
	}

	//Handle hits by lava
	if (IsHittingLava() && !m_Invulnerable)
	{
		Hit();
	}


	if (m_PlayerState == RUNNING)
	{
		m_pEmitter->SetEnabled(true);
		m_pParticleEmitter->SetMaxEnergy(0.5f);
		m_pParticleEmitter->SetMinEnergy(0.5f);
	}
	else
	{
		m_pParticleEmitter->SetMaxEnergy(0.0f);
		m_pParticleEmitter->SetMinEnergy(0.0f);
	}

	
	//Update animations depending on state
	UpdateAnimations();

	//Update last state
	m_LastPlayerState = m_PlayerState;

	// Animated lerp between up vectors
	if (m_Up != m_NewUp)
		m_Up = LerpVec3(m_Up, m_NewUp, gameContext.pGameTime->GetElapsed() * 1.95f);

	// Check if grounded
	UpdateGrounded();

	//Check if processing hit
	if (m_ProcessingHit)
	{
		m_ProcessingHitTimer += gameContext.pGameTime->GetElapsed();
		if (m_ProcessingHitTimer >= m_ProcessingHitTime)
		{
			m_ProcessingHit = false;
			m_ProcessingHitTimer = 0.0f;
		}
	}

	// Updating the up vector based on polygon below through raycast
	UpdateUp();
	m_pController->SetUpVector(m_Up.getNormalized());

	if (m_PlayerState != DYING && m_PlayerState != VICTORIOUS) {
		// Calculating the nw local axes taking spherical gravity and up of the surface into account
		XMVECTOR globalForward = { 1, 0, 0 };
		XMFLOAT3 fup = m_pController->GetUp();
		
		XMVECTOR localUp = XMLoadFloat3(&fup);
		XMVECTOR localRight = XMLoadFloat3(&GetTransform()->GetRight());
		XMVECTOR localForward = XMLoadFloat3(&GetTransform()->GetForward());
		localUp = XMVector3Normalize(localUp);
		localRight = XMVector3Normalize(localRight);
		localForward = XMVector3Normalize(localForward);

		//MOVEMENT
		if (!scene->IsShowingControls()) {
			// Move direction based on input
			m_MoveDirection.z = gameContext.pInput->IsActionTriggered(FORWARD) ? 1.0f : 0.0f;
			if (m_MoveDirection.z == 0) m_MoveDirection.z = -(gameContext.pInput->IsActionTriggered(BACKWARD) ? 1.0f : 0.0f);
			if (m_MoveDirection.z == 0) m_MoveDirection.z = gameContext.pInput->GetThumbstickPosition().y;
			m_MoveDirection.x = gameContext.pInput->IsActionTriggered(RIGHT) ? 1.0f : 0.0f;
			if (m_MoveDirection.x == 0) m_MoveDirection.x = -(gameContext.pInput->IsActionTriggered(LEFT) ? 1.0f : 0.0f);
			if (m_MoveDirection.x == 0) m_MoveDirection.x = gameContext.pInput->GetThumbstickPosition().x;

			std::cout << m_MoveDirection.x << " " << m_MoveDirection.y << " " << m_MoveDirection.z << std::endl;
			float angle = atan2f(m_MoveDirection.z, m_MoveDirection.x);
			angle = XMConvertToDegrees(angle);
			angle += 90.0f;
			if (m_MoveDirection.z == 0.0f && m_MoveDirection.x == 0.0f)
			{
				angle = m_LastAngle;
			}
			else
			{
				angle = LerpFloat(m_LastAngle, angle, gameContext.pGameTime->GetElapsed() * 10.5f);
				
			}
			m_pPlayerModelHolder->GetTransform()->Rotate(0.0f, -angle, 0.0f);
			m_LastAngle = angle;
			// Multilying by movement speed (we dont care about upwards movement yet)
			m_MoveDirection.x *= m_MoveSpeed;
			m_MoveDirection.z *= m_MoveSpeed;
		}

		

		// Add jump velocity if space is pessed and the GameCharacter is grounded
		if (gameContext.pInput->IsActionTriggered(JUMP) && m_IsGrounded)
		{
			m_MoveDirection.y = m_JumpSpeed;
			m_PlayerState = JUMPING;
			SoundManager::GetInstance()->GetSystem()->playSound(m_pJumpSound, NULL, false, &m_pChannel);
		}
		else if (m_IsGrounded)
		{
			m_MoveDirection.y = 0.0f;
			m_PlayerState = IDLE;
		}

		// Apply gravitational force
		m_MoveDirection.y -= m_Gravity * gameContext.pGameTime->GetElapsed();

		if ((m_MoveDirection.x != 0.0f || m_MoveDirection.z != 0.0f) && m_IsGrounded)
		{
			//m_LastPlayerState = m_PlayerState;
			m_PlayerState = RUNNING;
		}
		else if (m_MoveDirection.y > 0.0f)
		{
			m_PlayerState = JUMPING;
		}

		// Load the moveDirection in XMVECTOR
		XMVECTOR moveDir = XMLoadFloat3(&m_MoveDirection);
		// Apply the correct directions to movement based on directional vectors of GameCharacter controller
		XMVECTOR move = XMVectorGetByIndex(moveDir, 0) * localRight + XMVectorGetByIndex(moveDir, 2) * localForward + XMVectorGetByIndex(moveDir, 1) * localUp;


		// Storing the XMVECTOR
		move *= gameContext.pGameTime->GetElapsed();
		XMFLOAT3 movement{};
		XMStoreFloat3(&movement, move);

		// Passing movement to controller
		m_pController->Move(movement);

		//End movement

	}
	if (m_Hit && !m_Invulnerable && m_PlayerState != DYING && !scene->IsShowingControls() && m_PlayerState != VICTORIOUS)
	{
		HandleHit();
	}

	//Update model rotation
	UpdateRotation(gameContext.pGameTime->GetElapsed(), m_MoveDirection);


	if (m_CurrentHealth > 2 || m_CurrentHealth == 0 )
	{
		m_pHealthContainer->GetTransform()->Scale(0.55f, 0.55f, 1.0f);
	}
	else
	{
		
		if (m_CurrentHealth == 1) m_GrowValue += gameContext.pGameTime->GetElapsed() * m_CriticalMultiplier;
		else 
		{
			m_GrowValue += gameContext.pGameTime->GetElapsed() * m_DangerMultiplier;
		}
		if (m_GrowValue >= 360.0f)m_GrowValue = 0.0f;
		m_GrowRate = sin(m_GrowValue);
		if (m_GrowRate <= 0.45f)m_GrowRate = 0.45f;
		if (m_GrowRate >= 0.55f)m_GrowRate = 0.55f;

		m_pHealthContainer->GetTransform()->Scale(m_GrowRate, m_GrowRate, 1.0f);
	}

}

void GameCharacter::Draw(const GameContext & gameContext)
{
	UNREFERENCED_PARAMETER(gameContext);
	auto scene = static_cast<GameprojectScene*>(GetScene());
	if (scene->IsShowingControls() || scene->GameDone()) return;
	
	if (m_pFont->GetFontName() != L"" && m_CurrentHealth > 0)
	{
		
		DirectX::XMFLOAT3 posHealth = m_pHealthContainer->GetTransform()->GetPosition();
		TextRenderer::GetInstance()->DrawText(m_pFont, std::to_wstring(m_CurrentHealth), DirectX::XMFLOAT2(posHealth.x - 5.0f, posHealth.y - 40.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
		
		DirectX::XMFLOAT3 posStars = m_pStarCoinContainer->GetTransform()->GetPosition();
		TextRenderer::GetInstance()->DrawText(m_pFont, std::to_wstring(m_AmountOfStars), DirectX::XMFLOAT2(posStars.x + 45.0f, posStars.y - 30.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	
	}	

}

void GameCharacter::HandleHit()
{
	m_ProcessingHit = true;
	Blood* blood = new Blood(GetTransform()->GetPosition());
	GetScene()->AddChild(blood);
	m_Invulnerable = true;
	SoundManager::GetInstance()->GetSystem()->playSound(m_pHurtSound, NULL, false, &m_pChannel);
	UpdateHealth();
	CheckIfDeath();
	m_Hit = false;
}

void GameCharacter::AddStarScore(int value)
{
	m_AmountOfStars += value;
	if (m_AmountOfStars < 0) m_AmountOfStars = 0;
}

void GameCharacter::Reset()
{
	GetTransform()->Translate(m_xOffset, m_yOffset, m_zOffset);
	m_pPlayerModelHolder->GetTransform()->Rotate(0.0f, 180.0f, 0.0f);
	m_ProcessingHit = false;
	m_ProcessingHitTimer = 0.0f;
	m_AmountOfStars = 0;
	m_CurrentHealth = 3;
	m_pHealthSprite->SetTexture(*m_pHealthTextures[3]);
	m_PlayerState = IDLE;
	m_Invulnerable = false;
	m_InvulnerableTimer = 0.0f;
	GetTransform()->Rotate(0.0f, 0.0f, 0.0f);
	m_Up = physx::PxVec3(0, 1, 0);
	m_NewUp = physx::PxVec3(0, 1, 0);
	m_Hit = false;
}

void GameCharacter::UpdateAnimations()
{
	if(m_pAnimator->GetAnimationSpeed() >= 8.0f)m_pAnimator->SetAnimationSpeed(1.0f);
	if (!m_pAnimator) return;
	
	if (m_PlayerState == m_LastPlayerState) return;
	switch (m_PlayerState)
	{
	case GameCharacter::IDLE:
		m_pAnimator->SetAnimation(0);
		break;
	case GameCharacter::RUNNING:
		m_pAnimator->SetAnimation(1);
		break;
	case GameCharacter::JUMPING:
		m_pAnimator->SetAnimation(2);
		break;
	case GameCharacter::DYING:
		m_pAnimator->SetAnimation(5);
		break;
	case GameCharacter::VICTORIOUS:
		m_pAnimator->SetAnimation(3);
	default:
		break;
	}
}


void GameCharacter::UpdateHealth()
{
	--m_CurrentHealth;
	if (m_CurrentHealth < 1) m_CurrentHealth = 0;
	if(m_CurrentHealth >= 0)
		m_pHealthSprite->SetTexture(*m_pHealthTextures[m_CurrentHealth]);

	
}

void GameCharacter::UpdateInvulnerability(float deltaTime)
{
	m_InvulnerableTimer += deltaTime;
	if (m_InvulnerableTimer >= m_InvulnerableTime)
	{
		m_Invulnerable = false;
		m_InvulnerableTimer = 0.0f;
	}
}

void GameCharacter::UpdateRotation(float deltaTime, XMFLOAT3 moveDir)
{
	UNREFERENCED_PARAMETER(moveDir);
	DirectX::XMFLOAT3 up{ 0,1,0 };
	DirectX::XMFLOAT3 controllerUp{m_pController->GetUp()};
	DirectX::XMVECTOR controllerUpVector = DirectX::XMLoadFloat3(&controllerUp);
	DirectX::XMVECTOR upVector = DirectX::XMLoadFloat3(&up);

	//Calculate axis to rotate around
	DirectX::XMVECTOR axis = DirectX::XMVector3Cross(controllerUpVector, upVector);

	//Calculate dot product to get the angle
	DirectX::XMVECTOR dot = DirectX::XMVector3Dot(controllerUpVector, upVector);
	DirectX::XMFLOAT3 dotFloat;
	DirectX::XMStoreFloat3(&dotFloat, dot);

	float angle = acos(dotFloat.x);
	DirectX::XMMATRIX rotation1{};
	if (angle != 0.0f)
	{
		DirectX::XMVECTOR prevRot = DirectX::XMLoadFloat3(&m_PrevRot);
		rotation1 = DirectX::XMMatrixRotationAxis(axis, -angle);
		DirectX::XMVECTOR rotationQuaternion1 = DirectX::XMQuaternionRotationMatrix(rotation1);
		DirectX::XMVECTOR lerpedRotation = XMQuaternionSlerp(prevRot, rotationQuaternion1, deltaTime * 6);
		GetTransform()->Rotate(rotationQuaternion1, true);
		DirectX::XMStoreFloat3(&m_PrevRot, lerpedRotation);
	}

}

void GameCharacter::CheckIfDeath()
{
	if (m_CurrentHealth <= 0)
	{
		m_PlayerState = DYING;
		m_Invulnerable = true;

	}
}
