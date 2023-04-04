/***********************************************************************************
*   Copyright 2022 Marcos Sánchez Torrent.                                         *
*   All Rights Reserved.                                                           *
***********************************************************************************/

#pragma once

#ifndef GAL_WINDOW_MANAGER_H
#define GAL_WINDOW_MANAGER_H 1

#include "ImplPrerequisites.h"
#include "../Public/IWindowManager.h"
#include "../../GreaperCore/Public/IApplication.h"
#include "../../GreaperCore/Public/Concurrency.h"

namespace greaper::gal
{
	class WindowManager final : public IWindowManager
	{
		enum PropertiesIndices
		{
			Win32DPIScaling,

			COUNT
		};

		mutable RWMutex m_MonitorMutex;
		Vector<PMonitor> m_Monitors;
		sizet m_MainMonitor;

		mutable RWMutex m_WindowMutex;
		Vector<PWindow> m_Windows;

		WThreadManager m_ThreadManager;
		WThread m_MainThread;

		// ThreadManager getter and updater
		IInterface::ActivationEvt_t::HandlerType m_OnManagerActivation;
		void OnManagerActivation(bool active, IInterface* oldInterface, const PInterface& newInterface)noexcept;
		IApplication::OnInterfaceActivationEvent_t::HandlerType m_OnNewManager;
		void OnNewManager(const PInterface& newInterface)noexcept;

	public:
		WindowManager()noexcept = default;
		~WindowManager()noexcept = default;

		void OnInitialization()noexcept override;

		void OnDeinitialization()noexcept override;

		void OnActivation(const PInterface& oldDefault)noexcept override;

		void OnDeactivation(const PInterface& newDefault)noexcept override;

		void InitProperties()noexcept override;

		void DeinitProperties()noexcept override;
		
		void QueryMonitors();

		TResult<PWindow> CreateWindow(const WindowDesc& windowDesc)override;

		PMonitor GetMainMonitor() const override;
		
		void AccessMonitors(const std::function<void(CSpan<PMonitor>)>& accessFn) const override;

		void AccessWindows(const std::function<void(CSpan<PWindow>)>& accessFn) const override;

		WPtr<Win32DPIScalingProp_t> GetWin32DPIScaling()const noexcept override { return (WPtr<Win32DPIScalingProp_t>)m_Properties[(std::size_t)Win32DPIScaling]; }
	};
}

#endif /* GAL_WINDOW_MANAGER_H */