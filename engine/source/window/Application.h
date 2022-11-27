#pragma once
#include "connection/singletones/ConnectionManager.h"
#include "connection/UDPClient.h"
#include "connection/UDPServer.h"
#include "window.h"
#include "Renderer.h"
#include "InputState.h"
#include "../math/ray.h"
#include "../math/camera.h"
#include "../render/PostProcess.h"
#include "../render/PickableInstances.h"
#include <random>
#include <thread>


namespace engine::windows
{
	class Application
	{
		Window& wnd;
		Renderer& renderer;
		Camera camera;
		PostProcess m_postProcess;

		bool cursor_set = false;
		bool time_stopped = false;
		bool size_move = false;
		bool esc_time_stopped = false;
		std::chrono::steady_clock::time_point stop_time_point;
		std::chrono::steady_clock::time_point unstop_time_point;
		std::chrono::steady_clock::duration stop_duration = std::chrono::steady_clock::duration(0);

		std::chrono::steady_clock::time_point m_currentTime;

		bool need_to_move_camera = false;
		WORD mouse_x = 0, mouse_y = 0;
		InputState input_state;


		//---OBJECT MOVER---//
		ray ray_clicked_to_object;
		uint32_t clicked_object_transform_id;
		engine::MeshIntersection nearest_clicked_object;
		float distance_object_to_camera;
		bool need_to_move_object = false;
		//-----------------//

		//---OBJECT ROTATOR---//
		float object_rotate_angle = 0.2f;
		bool need_to_rotate_object = false;
		bool need_to_rotate_object_relative_to_camera_axes = false;
		//-----------------//

		//---CAMERA MOVER---//
		bool need_to_rotate_camera = false;
		bool need_to_speed_up = false;
		float camera_move_offset_val = 1.0f;
		float camera_angle_offset_val = 3.f;
		float acceleration = 5.f;
		Vec3 start_rotation = Vec3(0, 0, 1), end_rotation = Vec3(0, 0, 1), dir_rotation = Vec3(0, 0, 1);
		//-----------------//

		void RotateCamera();
		void MoveCamera(const Vec3& offset);
		void ProcessInput();
		void UpdateCurrentTime();

		std::default_random_engine rEngine;
		std::uniform_real_distribution<float> random;
		static const uint32_t NUM_PICKABLE_INSTANCES = 24;

		void SetPickableInstances(const Vec3& lb, const Vec3& rt, uint32_t num, float size);
		void SetPickableInstances(const std::array<PickableInstances::Instance, NUM_PICKABLE_INSTANCES>& arrInstances);
		void CheckPickableInstances();

		void SendData();
		void RecvData();
		void UpdateDataUsingRecvData();

		uint32_t m_playerID;

		std::vector<USHORT> m_opponentsPorts;
		std::vector<std::string> m_opponentsAddresses;

		bool m_program_exist = true;
		bool m_connected = false;
		bool m_scene_inited = false;

		std::vector<uint32_t> m_otherEntranceIDs;

		struct PlayerData
		{
			uint32_t m_transformID;
			uint32_t m_playerScore;
		};

		std::unordered_map<uint32_t, PlayerData> m_playersData;
		uint32_t m_yourPort;

		struct GameData
		{
			Matr<4> m_playerTramsformMatrix;
			int32_t m_entranceID;

			bool m_pickableInstancesWereCreated;
			uint32_t m_numNonPickedInstances;
			uint32_t m_playerScore;
			std::array<PickableInstances::Instance, NUM_PICKABLE_INSTANCES> m_instances;

			GameData() {}
		};

		GameData m_sendGameData;
		GameData m_recvGameData;

		engine::connection::UDPServer m_server;
		engine::connection::UDPClient m_client;
		std::thread m_recvThread;
		uint32_t m_numOtherPlayers;
		bool m_single_player;

	public:
		float delta_time = 0;
		Application(engine::windows::Window& _wnd, engine::windows::Renderer& _scene) : wnd(_wnd), renderer(_scene) {}

		void OnKeyDown(WPARAM key);
		void OnKeyUp(WPARAM key);

		void OnMouseMove(WORD x, WORD y);

		void OnEnterSizeMove();
		void OnExitSizeMove();
		void OnSetCursor(LPARAM lparam);

		void OnMouseWheel(short wheel_data);

		void Draw();
		void OnChangeWindowSize();
		void Init();
		void InitConnection(LPWSTR cmdline);

		void OnQuit();
	};
}