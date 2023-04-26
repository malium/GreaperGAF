/***********************************************************************************
*   Copyright 2022 Marcos Sánchez Torrent.                                         *
*   All Rights Reserved.                                                           *
***********************************************************************************/

#ifndef GAL_VK_WIN_WINDOW_IMPL_H
#define GAL_VK_WIN_WINDOW_IMPL_H 1

#include "../ImplPrerequisites.h"
#include "../../Public/Win/VkWinWindow.h"

namespace greaper::gal
{
	class VkWinWindowImpl final : public VkWinWindow
	{
	public:
		EmptyResult Create(const WindowDesc& windowDesc)noexcept override;

		// Inherited via VkWinWindow
		EmptyResult ChangeWindowSize(math::Vector2i size) override;
		EmptyResult ChangeWindowPosition(math::Vector2i size) override;
		EmptyResult ChangeWindowPosition(AnchoredPosition_t anchor) override;
		void SetWindowTitle(StringView title) override;
		EmptyResult ChangeWindowMode(WindowMode_t mode) override;
		EmptyResult ChangeWindowState(WindowState_t state) override;
		void ShowWindow() override;
		void HideWindow() override;
		void RequestFocus() override;
		void EnableResizing(bool enable) override;
		void SetResizingAspectRatio(math::Vector2i aspectRatio, bool changeCurrent) override;
		void SetMaxWindowSize(math::Vector2i maxSize, bool changeCurrent) override;
		void SetMinWindowSize(math::Vector2i minSize, bool changeCurrent) override;
		String GetClipboardText() const override;
		EmptyResult SetClipboardText(StringView text) override;
		bool HasClipboardText() override;
		void PollEvents()override;
		void SwapWindow() override;
		void CloseWindow() override;
		RenderBackend_t GetRenderBackend() const override;
	};
}

#endif /* GAL_VK_WIN_WINDOW_IMPL_H */