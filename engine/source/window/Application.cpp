#include "Application.h"
#include "../render/singletones/SamplerManager.h"
#include "../render/singletones/LightSystem.h"
#include "../render/singletones/ModelManager.h"
#include "../render/singletones/MeshSystem.h"
#include "../render/singletones/ShadowManager.h"
#include <thread>

const float p_near = 0.01f, p_far = 100.f, fovy = M_PI / 3.f;

namespace engine::windows
{
	void Application::Draw()
	{
		if (m_connected)
		{
			if (!m_sendGameData.m_pickableInstancesWereCreated)
			{
				if (m_recvGameData.m_pickableInstancesWereCreated)
				{
					SetPickableInstances(m_recvGameData.m_instances);
					m_sendGameData.m_pickableInstancesWereCreated = true;
				}
				else
				{
					USHORT minPort = m_sendGameData.m_entranceID;
					for (uint32_t i = 0; i < m_opponentsPorts.size(); ++i)
					{
						if (m_opponentsPorts[i] < minPort)
							minPort = m_opponentsPorts[i];
					}
					if (minPort == m_sendGameData.m_entranceID)
					{
						SetPickableInstances(Vec3(-10, 0.1f, -10), Vec3(10, 0.1f, 10), NUM_PICKABLE_INSTANCES, 0.3f);
						m_sendGameData.m_pickableInstancesWereCreated = true;
						SendData();
					}
				}
			}
		}
		ProcessInput();
		UpdateCurrentTime();
		CheckPickableInstances();

		SendData();


		wnd.BeginFrame();
		renderer.Render(wnd.m_renderTargetView, camera, m_postProcess, m_currentTime, delta_time);
		wnd.EndFrame();
	}

	void Application::OnChangeWindowSize()
	{
		wnd.OnResize();

		need_to_rotate_camera = false;
		float aspect = float(wnd.screen.right) / wnd.screen.bottom;
		camera.updateAspect(aspect);
		renderer.need_to_resize_RTV = true;
		Draw();
	}

	void Application::Init()
	{
		m_currentTime = std::chrono::steady_clock::now();
		rEngine = std::default_random_engine(std::chrono::duration_cast<std::chrono::duration<uint32_t>>(std::chrono::steady_clock::now().time_since_epoch()).count());

		renderer.Init(1u);
		m_postProcess.Init(1.5f);
		ShadowManager::instance().SetDirectionalLightDSResolution(2048.f);
		ShadowManager::instance().SetDirectionalLightShadowDistance(40.f);
		ShadowManager::instance().SetPointLightDSResolution(1024.f);


		float aspect = float(wnd.screen.right) / wnd.screen.bottom;
		camera = Camera(fovy, aspect, p_near, p_far);
		LightSystem::instance().setDirectionalLightFrustum(camera);
		camera.setWorldOffset(Vec3(0, 2, -2));

		//LIGHTS
		{
			engine::LightSystem::instance().addPointLight(Vec3(2, 2, 2), 1.7f, Vec3(0, 1.7f, 5.3f), 0.25f,
				engine::ModelManager::instance().GetUnitSphereModel());
			engine::LightSystem::instance().addPointLight(Vec3(3, 0.1f, 3), 1.4f, Vec3(-5, 1.f, -4.3f), 0.15f,
				engine::ModelManager::instance().GetUnitSphereModel());
			engine::LightSystem::instance().addPointLight(Vec3(0.1, 1.f, 0.1f), 2.f, Vec3(5, 0.5f, 0.3f), 0.15f,
				engine::ModelManager::instance().GetUnitSphereModel());

			engine::LightSystem::instance().addDirectionalLight(Vec3(1.f, 1.f, 1.f), Vec3(-1, 1, -1));

			engine::ShadowManager::instance().UpdatePointLightShadowResources(engine::LightSystem::instance().getPointLights().size());
			engine::ShadowManager::instance().UpdateDirectionalLightShadowResources(engine::LightSystem::instance().getDirectionalLights().size());
		}

		//SKY
		{
			renderer.m_sky.Init();
			renderer.m_sky.m_texture.Load(L"engine/assets/Sky/night_street.dds");
			renderer.m_sky.SetEnvironment(L"engine/assets/Sky/night_street_irradiance.dds",
				L"engine/assets/Sky/night_street_reflectance.dds",
				L"engine/assets/Sky/night_street_reflection.dds");
		}

		//CUBES
		{
			std::vector<OpaqueInstances::Material> brick(1);
			brick[0].m_colorMap.Load(L"engine/assets/Brick/Brick_Wall_012_COLOR.dds");
			brick[0].m_normalMap.Load(L"engine/assets/Brick/Brick_Wall_012_NORM.dds");
			brick[0].m_roughnessMap.Load(L"engine/assets/Brick/Brick_Wall_012_ROUGH.dds");
			brick[0].m_materialConstantBuffer = MaterialConstantBuffer(false, true, true, false, 0.02f);

			std::vector<OpaqueInstances::Material> stone(1);
			stone[0].m_colorMap.Load(L"engine/assets/Stone/Stone_COLOR.dds");
			stone[0].m_normalMap.Load(L"engine/assets/Stone/Stone_NORM.dds");
			stone[0].m_roughnessMap.Load(L"engine/assets/Stone/Stone_ROUGH.dds");
			stone[0].m_materialConstantBuffer = MaterialConstantBuffer(false, true, true, true, 0.02f);

			Transform transform = Transform::Identity();
			transform.SetScale({ 20.f, 3.f, 20.f });
			transform.SetPosition({ 0, -2, 0 });
			engine::MeshSystem::instance().addInstance(engine::ModelManager::instance().GetUnitCubeModel(), stone, OpaqueInstances::Instance(transform, false));

			
			for (uint32_t i = 0; i <= 10; ++i)
			{
				for (uint32_t j = 0; j <= 10; ++j)
				{
					transform.SetScale({ 0.1f, 1.3f + 0.3f * sinf(float(i+j)), 0.1f});
					transform.SetPosition({ float(i*2.f) - 10.f, 0.f, float(j*2.f) - 10.f });
					engine::MeshSystem::instance().addInstance(engine::ModelManager::instance().GetUnitCubeModel(), (j+i) % 2 == 0 ? brick : stone, OpaqueInstances::Instance(transform, true));
				}
			}
		}

		//PLAYERS
		{
			Transform transform = Transform::Identity();
			transform.SetScale({ 0.5f, 1.f, 0.5f });
			transform.SetPosition(camera.position());

			m_playerID = m_playersData[m_yourPort].m_transformID =
				engine::MeshSystem::instance().addInstance(
					engine::ModelManager::instance().GetUnitCubeModel(),
					EmissiveInstances::Instance({ 0,1,0 }, transform));

			m_playersData[m_yourPort].m_playerScore = 0;
			for (uint32_t i = 0; i < m_numOtherPlayers; ++i)
			{
				m_playersData[m_opponentsPorts[i]].m_transformID =
					engine::MeshSystem::instance().addInstance(
						engine::ModelManager::instance().GetUnitCubeModel(),
						EmissiveInstances::Instance({ 1,1,0 }, transform));
				m_playersData[m_opponentsPorts[i]].m_playerScore = 0;
			}

			m_recvThread = std::thread([&]() {RecvData(); });
			m_recvThread.detach();
		}
	}

	void Application::InitConnection(LPWSTR cmdLine)
	{
		int argc;
		LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);

		std::vector<std::string> args(argc);
		int strSize;
		for (int i = 0; i < argc; i++)
		{
			strSize = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
			std::unique_ptr<char> cmdParameter(new char[strSize]);
			WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, cmdParameter.get(), strSize, nullptr, nullptr);
			args[i] = cmdParameter.get();
		}
		m_numOtherPlayers = std::atoi(args[1].c_str());
		m_single_player = !m_numOtherPlayers;

		if (!m_single_player)
		{
			m_yourPort = std::atoi(args[2].c_str());
			m_server.Init(m_yourPort);
			m_opponentsPorts.resize(m_numOtherPlayers);
			m_opponentsAddresses.resize(m_numOtherPlayers);
			m_otherEntranceIDs.resize(m_numOtherPlayers);

			for (uint32_t i = 0; i < m_numOtherPlayers; ++i)
			{
				m_opponentsPorts[i] = std::atoi(args[3 + i + i].c_str());
				m_otherEntranceIDs[i] = m_opponentsPorts[i];
				m_opponentsAddresses[i] = args[3 + i + i + 1];
			}

			m_client.Init(m_numOtherPlayers, m_opponentsPorts, m_opponentsAddresses);

			m_sendGameData.m_playerScore = 0;
			m_sendGameData.m_numNonPickedInstances = 0;
			m_sendGameData.m_pickableInstancesWereCreated = false;
			m_sendGameData.m_entranceID = m_yourPort;

			m_recvGameData.m_playerScore = 0;
			m_recvGameData.m_numNonPickedInstances = 0;
			m_recvGameData.m_pickableInstancesWereCreated = false;
			m_recvGameData.m_entranceID = -1;

			m_recvGameData.m_playerTramsformMatrix = Matr<4>::identity();
		}
	}

	void Application::OnQuit()
	{
		m_program_exist = false;
		m_client.Deinit();
		m_server.Deinit();
	}

	void Application::UpdateCurrentTime()
	{
		if (time_stopped)
			m_currentTime = stop_time_point;
		else
			m_currentTime = std::chrono::steady_clock::now() - stop_duration;
	}

	void Application::SetPickableInstances(const Vec3& lb, const Vec3& rt, uint32_t num, float size)
	{
		for (uint32_t i = 0; i < num; ++i)
		{
			engine::MeshSystem::instance().addInstance(PickableInstances::Instance(
				{ random(rEngine,std::uniform_real<float>::param_type(lb.e[0], rt.e[0])), lb.e[1], random(rEngine,std::uniform_real<float>::param_type(lb.e[2], rt.e[2])) },
				size));
		}
	}

	void Application::SetPickableInstances(const std::array<PickableInstances::Instance, NUM_PICKABLE_INSTANCES>& arrInstances)
	{
		for (uint32_t i = 0; i < NUM_PICKABLE_INSTANCES; ++i)
		{
			engine::MeshSystem::instance().addInstance(PickableInstances::Instance(arrInstances[i].instancePos, arrInstances[i].instanceSize));
		}
	}

	void Application::CheckPickableInstances()
	{
		auto size = MeshSystem::instance().pickableInstances.m_instances.size();
		if (size == 0)
			return;

		auto& instance = MeshSystem::instance().pickableInstances.m_instances;
		auto iterator =
			std::remove_if(instance.begin(), instance.end(), [&](auto inst) {
			return Vec3(camera.position() - inst.instancePos).length() <= inst.instanceSize / 2.f + 0.2f;
				});
		instance.erase(iterator, instance.end());
		m_sendGameData.m_playerScore += size - instance.size();
		m_playersData[m_yourPort].m_playerScore = m_sendGameData.m_playerScore;
	}

	void Application::SendData()
	{
		auto& instances = MeshSystem::instance().pickableInstances.m_instances;
		std::copy(instances.begin(), instances.end(), 
			m_sendGameData.m_instances.begin());
		m_sendGameData.m_numNonPickedInstances = instances.size();
		m_sendGameData.m_playerTramsformMatrix = 
			TransformSystem::instance().m_transforms[m_playerID].transform;

		m_client.SendData<GameData>(m_sendGameData, sizeof(m_sendGameData));
	}

	void Application::RecvData()
	{
		while (m_program_exist)
		{
			m_server.RecvData<GameData>(m_recvGameData, sizeof(m_recvGameData));
			if (!m_connected)
			{
				auto it = std::remove_if(m_otherEntranceIDs.begin(), 
					m_otherEntranceIDs.end(), [&](auto& id) 
					{return m_recvGameData.m_entranceID == id; });

				m_otherEntranceIDs.erase(it, m_otherEntranceIDs.end());
				if (m_otherEntranceIDs.size() == 0)
				{
					m_connected = true;
				}
				else
					continue;
			}
			UpdateDataUsingRecvData();
		}
	}

	void Application::UpdateDataUsingRecvData()
	{
		TransformSystem::instance().
			m_transforms[m_playersData[m_recvGameData.m_entranceID].m_transformID].transform =
			m_recvGameData.m_playerTramsformMatrix;

		m_playersData[m_recvGameData.m_entranceID].m_playerScore = m_recvGameData.m_playerScore;

		if (m_sendGameData.m_pickableInstancesWereCreated && 
			m_recvGameData.m_pickableInstancesWereCreated)
		{
			auto& instances = MeshSystem::instance().pickableInstances.m_instances;
			if (m_recvGameData.m_numNonPickedInstances < 
				m_sendGameData.m_numNonPickedInstances)
			{
				instances.resize(m_recvGameData.m_numNonPickedInstances);
				for (uint32_t i = 0; i < m_recvGameData.m_numNonPickedInstances; ++i)
					instances[i] = m_recvGameData.m_instances[i];
			}
			
			if ((m_recvGameData.m_numNonPickedInstances == 0 && 
				m_sendGameData.m_numNonPickedInstances == 0))
			{
				time_stopped = true;
				m_program_exist = false;

				int32_t winnerID = m_sendGameData.m_entranceID;
				int32_t drawCount = 0;
				for (auto& p : m_playersData)
				{
					if (p.first == m_sendGameData.m_entranceID)
						continue;
					if (p.second.m_playerScore > m_playersData[winnerID].m_playerScore)
						winnerID = p.first;
					else if (p.second.m_playerScore == m_playersData[winnerID].m_playerScore)
						drawCount += 1;
				}

				if (drawCount == m_numOtherPlayers)
					MessageBox(nullptr, 
						(std::wstring(L"DRAW\nYOUR SCORE : ") + std::to_wstring(m_sendGameData.m_playerScore)).c_str()
						, L"DRAW", MB_ICONINFORMATION);
				else if (m_sendGameData.m_entranceID == winnerID)
					MessageBox(nullptr,
						(std::wstring(L"YOU WON!!!\nYOUR SCORE : ") + std::to_wstring(m_sendGameData.m_playerScore)).c_str(),
						L"VICTORY", MB_ICONINFORMATION);
				else MessageBox(nullptr, 
					(std::wstring(L"YOU LOSE!!!\nYOUR SCORE : ") + std::to_wstring(m_sendGameData.m_playerScore)).c_str(),
					L"DEFEAT", MB_ICONINFORMATION);
			}
		}
	}

	void Application::ProcessInput()
	{
		Vec3 offset = Vec3(0, 0, 0);
		if (input_state[VK_ESCAPE])
		{
			if (!size_move)
			{
				if (time_stopped)
				{
					time_stopped = false;
					esc_time_stopped = false;
				}
				else
				{
					time_stopped = true;
					esc_time_stopped = true;
				}
				input_state[VK_ESCAPE] = false;
			}
		}
		if (!time_stopped)
		{
			//MOVEMENT
			{
				if (input_state[VK_SHIFT])
				{
					need_to_speed_up = true;
				}
				else
				{
					need_to_speed_up = false;
				}
				if (input_state['A'])
				{
					offset.e[0] -= camera_move_offset_val * ((need_to_speed_up) ? acceleration : 1.f);
					need_to_move_camera = true;
				}
				if (input_state['D'])
				{
					offset.e[0] += camera_move_offset_val * ((need_to_speed_up) ? acceleration : 1.f);
					need_to_move_camera = true;
				}
				if (input_state['W'])
				{
					offset.e[2] += camera_move_offset_val * ((need_to_speed_up) ? acceleration : 1.f);
					need_to_move_camera = true;
				}
				if (input_state['S'])
				{
					offset.e[2] -= camera_move_offset_val * ((need_to_speed_up) ? acceleration : 1.f);
					need_to_move_camera = true;
				}
				if (input_state['Q'])
				{
					need_to_move_camera = true;
					offset.e[1] -= camera_move_offset_val * ((need_to_speed_up) ? acceleration : 1.f);
				}
				if (input_state['E'])
				{
					need_to_move_camera = true;
					offset.e[1] += camera_move_offset_val * ((need_to_speed_up) ? acceleration : 1.f);
				}
			}

			//SAMPLE_FILTRATION
			{
				if (input_state['1'])
				{
					engine::SamplerManager::instance().SetGlobalSamplerState("ss_a");
				}
				if (input_state['2'])
				{
					engine::SamplerManager::instance().SetGlobalSamplerState("ss_mmmp");
				}
				if (input_state['3'])
				{
					engine::SamplerManager::instance().SetGlobalSamplerState("ss_mpmlmp");
				}
				if (input_state['4'])
				{
					engine::SamplerManager::instance().SetGlobalSamplerState("ss_mmlmp");
				}
				if (input_state['5'])
				{
					engine::SamplerManager::instance().SetGlobalSamplerState("ss_mmml");
				}
			}

			//LIGHTING
			{
				if (input_state[VK_OEM_PLUS])
				{
					m_postProcess.EV100 += 1.f * delta_time;
				}
				if (input_state[VK_OEM_MINUS])
				{
					m_postProcess.EV100 -= 1.f * delta_time;
				}
			}


			//OBJECT_ROTATION
			{
				if (input_state['C'])
				{
					if (need_to_rotate_object_relative_to_camera_axes)
					{
						need_to_rotate_object_relative_to_camera_axes = false;
						input_state['C'] = false;
					}
					else
					{
						need_to_rotate_object_relative_to_camera_axes = true;
						input_state['C'] = false;
					}
				}

				if (need_to_move_object)
				{
					Angles rotate_angles;
					if (input_state[VK_LEFT])
					{
						rotate_angles.yaw -= object_rotate_angle;
						need_to_rotate_object = true;
					}
					if (input_state[VK_RIGHT])
					{
						rotate_angles.yaw += object_rotate_angle;
						need_to_rotate_object = true;
					}
					if (input_state[VK_UP])
					{
						rotate_angles.pitch -= object_rotate_angle;
						need_to_rotate_object = true;
					}
					if (input_state[VK_DOWN])
					{
						rotate_angles.pitch += object_rotate_angle;
						need_to_rotate_object = true;
					}
					if (input_state[VK_OEM_COMMA])
					{
						rotate_angles.roll -= object_rotate_angle;
						need_to_rotate_object = true;
					}
					if (input_state[VK_OEM_PERIOD])
					{
						rotate_angles.roll += object_rotate_angle;
						need_to_rotate_object = true;
					}
					if (need_to_rotate_object)
					{
						rotate_angles.pitch *= delta_time; rotate_angles.roll *= delta_time; rotate_angles.yaw *= delta_time;
						if (need_to_rotate_object_relative_to_camera_axes)
						{
							engine::TransformSystem::instance().m_transforms[clicked_object_transform_id].Rotate(rotate_angles, camera.right(), camera.top(), camera.forward());
						}
						else
						{
							auto& transform = engine::TransformSystem::instance().m_transforms[clicked_object_transform_id];
							transform.Rotate(rotate_angles, transform.right(), transform.top(), transform.forward());
						}
						need_to_rotate_object = false;
					}
				}
			}

			if (need_to_rotate_camera)
				RotateCamera();
			if (need_to_move_camera)
			{
				MoveCamera(offset * delta_time);
				need_to_move_camera = false;
			}
		}
	}

	void Application::OnKeyDown(WPARAM key)
	{
		input_state[key] = true;
	}

	void Application::OnKeyUp(WPARAM key)
	{
		input_state[key] = false;
	}

	void Application::OnMouseMove(WORD x, WORD y)
	{
		if (!time_stopped)
		{
			if (!cursor_set)
			{
				need_to_rotate_camera = true;
				mouse_x = x;
				mouse_y = y;
				dir_rotation = delta_time * M_PI * (Vec3(wnd.screen.right / 2, wnd.screen.bottom / 2, 0) - Vec3(x, y, 0)) / 20;

				POINT p;
				p.x = wnd.screen.right / 2;
				p.y = wnd.screen.bottom / 2;

				ClientToScreen(wnd.m_wndHandle, &p);
				SetCursorPos(p.x, p.y);
				cursor_set = true;
			}
			else
				cursor_set = false;
		}
	}

	void Application::MoveCamera(const Vec3& offset)
	{
		camera.addRelativeOffset(offset);
		camera.updateMatrices();
		TransformSystem::instance().m_transforms[m_playerID].TranslateRelative(offset);
	}

	void Application::OnEnterSizeMove()
	{
		if (!size_move)
		{
			time_stopped = true;
			size_move = true;
			delta_time = 0;
			stop_time_point = m_currentTime;
		}
	}

	void Application::OnExitSizeMove()
	{
		if (size_move)
		{
			size_move = false;

			if (!esc_time_stopped)
				time_stopped = false;

			unstop_time_point = std::chrono::steady_clock::now();
			stop_duration = unstop_time_point - stop_time_point;
		}
	}

	void Application::OnSetCursor(LPARAM lparam)
	{
		if (LOWORD(lparam) == HTCLIENT && !time_stopped)
		{
			SetCursor(NULL);
		}
		else
		{
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
		}
	}

	void Application::OnMouseWheel(short wheel_data)
	{
		short count = wheel_data / WHEEL_DELTA;
		if (count < 0)
			camera_move_offset_val /= abs(count) * 1.1f;
		else
			camera_move_offset_val *= abs(count) * 1.1f;
	}

	void Application::RotateCamera()
	{
		camera.addRelativeAngles(Angles(0, dir_rotation.e[1], dir_rotation.e[0]));
		camera.updateMatrices();
		TransformSystem::instance().m_transforms[m_playerID].Rotate(Angles(0, dir_rotation.e[1], dir_rotation.e[0]));
		need_to_rotate_camera = false;
	}
}