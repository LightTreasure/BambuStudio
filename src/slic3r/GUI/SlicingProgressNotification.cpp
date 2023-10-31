#include "SlicingProgressNotification.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace Slic3r { namespace GUI {

namespace {
	inline void push_style_color(ImGuiCol idx, const ImVec4& col, bool fading_out, float current_fade_opacity)
	{
		if (fading_out)
			ImGui::PushStyleColor(idx, ImVec4(col.x, col.y, col.z, col.w * current_fade_opacity));
		else
			ImGui::PushStyleColor(idx, col);
	}
}

static constexpr int   BEFORE_COMPLETE_DURATION = 3000; //ms
static constexpr int   REFRESH_TIMEOUT = 100;           //ms

void NotificationManager::SlicingProgressNotification::on_change_color_mode(bool is_dark)
{
	PopNotification::on_change_color_mode(is_dark);
	m_dailytips_panel->on_change_color_mode(is_dark);
}

void NotificationManager::SlicingProgressNotification::init()
{
	if (m_sp_state == SlicingProgressState::SP_PROGRESS) {
		PopNotification::init();
		if (m_endlines.empty()) {
			m_endlines.push_back(0);
		}
		if (m_lines_count >= 2) {
			m_lines_count = 3;
			m_multiline = true;
			while (m_endlines.size() < 3)
				m_endlines.push_back(m_endlines.back());
		}
		else {
			m_lines_count = 2;
			m_endlines.push_back(m_endlines.back());
			m_multiline = false;
		}
		if (m_state == EState::Shown)
			m_state = EState::NotFading;
	}
	else {
		PopNotification::init();
	}

}

bool NotificationManager::SlicingProgressNotification::set_progress_state(float percent)
{
	if (percent < 0.f)
		return true;//set_progress_state(SlicingProgressState::SP_CANCELLED);
	else if (percent >= 1.f) {
		if (m_dailytips_panel->is_expanded()) {
			m_before_complete_start = GLCanvas3D::timestamp_now();
			return set_progress_state(SlicingProgressState::SP_BEFORE_COMPLETED);
		}
		else
			return set_progress_state(SlicingProgressState::SP_COMPLETED);
	}
	else
		return set_progress_state(SlicingProgressState::SP_PROGRESS, percent);
}

bool NotificationManager::SlicingProgressNotification::set_progress_state(NotificationManager::SlicingProgressNotification::SlicingProgressState state, float percent/* = 0.f*/)
{
	switch (state)
	{
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_NO_SLICING:
        m_state = EState::Hidden;
        set_percentage(-1);
        m_has_print_info = false;
        set_export_possible(false);
        m_sp_state             = state;
        return true;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_BEGAN:
		m_state = EState::Hidden;
		set_percentage(-1);
		m_has_print_info = false;
		set_export_possible(false);
		m_sp_state = state;
        m_current_fade_opacity = 1;
		return true;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_PROGRESS:
		if ((m_sp_state != SlicingProgressState::SP_BEGAN && m_sp_state != SlicingProgressState::SP_PROGRESS) || percent < m_percentage)
			return false;
		set_percentage(percent);
		if (m_sp_state == SlicingProgressState::SP_BEGAN) {
			wxGetApp().plater()->get_dailytips()->close();
			m_dailytips_panel->retrieve_data_from_hint_database(HintDataNavigation::Random);
		}
		m_sp_state = state;
        m_current_fade_opacity = 1;
		return true;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_CANCELLED:
		set_percentage(-1);
		m_has_print_info = false;
		set_export_possible(false);
		m_sp_state = state;
		return true;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_BEFORE_COMPLETED:
		if (m_sp_state != SlicingProgressState::SP_BEGAN && m_sp_state != SlicingProgressState::SP_PROGRESS)
			return false;
		m_has_print_info = false;
		// m_export_possible is important only for SP_PROGRESS state, thus we can reset it here
		set_export_possible(false);
		m_sp_state = state;
		return true;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_COMPLETED:
		if (m_sp_state != SlicingProgressState::SP_BEGAN && m_sp_state != SlicingProgressState::SP_PROGRESS && m_sp_state != SlicingProgressState::SP_BEFORE_COMPLETED)
			return false;
		set_percentage(1);
		m_has_print_info = false;
		// m_export_possible is important only for SP_PROGRESS state, thus we can reset it here
		set_export_possible(false);
		m_sp_state = state;
		return true;
	default:
		break;
	}
	return false;
}

void NotificationManager::SlicingProgressNotification::set_status_text(const std::string& text)
{
	switch (m_sp_state)
	{
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_NO_SLICING:
		m_state = EState::Hidden;
		break;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_PROGRESS:
	{
		NotificationData data{ NotificationType::SlicingProgress, NotificationLevel::ProgressBarNotificationLevel, 0, text + "." };
		update(data);
		m_state = EState::NotFading;
	}
		break;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_CANCELLED:
	{
		NotificationData data{ NotificationType::SlicingProgress, NotificationLevel::ProgressBarNotificationLevel, 0, text };
		update(data);
		m_state = EState::Shown;
	}
		break;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_BEFORE_COMPLETED:
	{
		NotificationData data{ NotificationType::SlicingProgress, NotificationLevel::ProgressBarNotificationLevel, 0,  _u8L("Slice ok.") };
		update(data);
		m_state = EState::Shown;
	}
		break;
	case Slic3r::GUI::NotificationManager::SlicingProgressNotification::SlicingProgressState::SP_COMPLETED:
	{
		NotificationData data{ NotificationType::SlicingProgress, NotificationLevel::ProgressBarNotificationLevel, 0,  _u8L("Slice ok.") };
		update(data);
		m_state = EState::Shown;
	}
		break;
	default:
		break;
	}
}

void NotificationManager::SlicingProgressNotification::set_print_info(const std::string& info)
{
	if (m_sp_state != SlicingProgressState::SP_COMPLETED) {
		set_progress_state (SlicingProgressState::SP_COMPLETED);
	} else {
		m_has_print_info = true;
		m_print_info = info;
	}
}

void NotificationManager::SlicingProgressNotification::set_sidebar_collapsed(bool collapsed)
{
	m_sidebar_collapsed = collapsed;
	if (m_sp_state == SlicingProgressState::SP_COMPLETED && collapsed)
		m_state = EState::NotFading;
}

void NotificationManager::SlicingProgressNotification::on_cancel_button()
{
	if (m_cancel_callback){
		if (!m_cancel_callback()) {
			set_progress_state(SlicingProgressState::SP_NO_SLICING);
		}
	}
}

int NotificationManager::SlicingProgressNotification::get_duration()
{
	if (m_sp_state == SlicingProgressState::SP_CANCELLED)
		return 3;
	else if (m_sp_state == SlicingProgressState::SP_COMPLETED && !m_sidebar_collapsed)
		return 5;
	else
		return 0;
}

bool  NotificationManager::SlicingProgressNotification::update_state(bool paused, const int64_t delta)
{
	bool ret = PopNotification::update_state(paused, delta);
	if (m_sp_state == SlicingProgressState::SP_BEFORE_COMPLETED || m_sp_state == SlicingProgressState::SP_COMPLETED)
		ret = true;

	// sets Estate to hidden
	if (get_state() == PopNotification::EState::ClosePending || get_state() == PopNotification::EState::Finished)
		set_progress_state(SlicingProgressState::SP_NO_SLICING);
	return ret;
}

void NotificationManager::SlicingProgressNotification::render(GLCanvas3D& canvas, float initial_y, bool move_from_overlay, float overlay_width, float right_margin)
{
	if (m_state == EState::Unknown || m_state == PopNotification::EState::Hovered)
		init();

	ImGuiWrapper& imgui = *wxGetApp().imgui();
	float scale = imgui.get_font_size() / 15.0f;

	bool fading_pop = false;
	if (m_state == EState::FadingOut) {
		push_style_color(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg), true, m_current_fade_opacity);
		push_style_color(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text), true, m_current_fade_opacity);
		push_style_color(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered), true, m_current_fade_opacity);
		fading_pop = true;
	}
	use_bbl_theme();
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * scale, 0));
	if (m_sp_state == SlicingProgressNotification::SlicingProgressState::SP_COMPLETED || m_sp_state == SlicingProgressNotification::SlicingProgressState::SP_CANCELLED)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,m_WindowRadius / 4);
		m_is_dark ? push_style_color(ImGuiCol_Border, { 62 / 255.f, 62 / 255.f, 69 / 255.f, 1.f }, true, m_current_fade_opacity) : push_style_color(ImGuiCol_Border, m_CurrentColor, true, m_current_fade_opacity);
	}
	else {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		push_style_color(ImGuiCol_Border, { 0, 0, 0, 0 }, true, m_current_fade_opacity);
	}

	const ImVec2 progress_child_window_padding = ImVec2(25.f, 5.f) * scale;
	const ImVec2 dailytips_child_window_padding = ImVec2(25.f, 2.f) * scale;
	const ImVec2 bottom_padding = ImVec2(25.f, 20.f) * scale;
	const float  progress_panel_width = (m_window_width - 2 * progress_child_window_padding.x);
	const float  progress_panel_height = (78.0f * scale);
	const float  dailytips_panel_width = (m_window_width - 2 * dailytips_child_window_padding.x);
	const float  dailytips_panel_height = (395.0f * scale);

	Size cnv_size = canvas.get_canvas_size();
	float right_gap = right_margin + (move_from_overlay ? overlay_width + m_line_height * 5 : 0);
	ImVec2 window_pos((float)cnv_size.get_width() - right_gap - m_window_width, (float)cnv_size.get_height() - m_top_y);
	imgui.set_next_window_pos(window_pos.x, window_pos.y, ImGuiCond_Always, 0.0f, 0.0f);
	// dynamically resize window by progress state
	if (m_sp_state == SlicingProgressNotification::SlicingProgressState::SP_COMPLETED || m_sp_state == SlicingProgressNotification::SlicingProgressState::SP_CANCELLED)
		m_window_height = 64.0f * scale;
	else
		m_window_height = progress_panel_height + m_dailytips_panel->get_size().y + progress_child_window_padding.y + dailytips_child_window_padding.y + bottom_padding.y;
	m_top_y = initial_y + m_window_height;
	ImGui::SetNextWindowSizeConstraints(ImVec2(m_window_width, m_window_height), ImVec2(m_window_width, m_window_height));

	// name of window indentifies window - has to be unique string
	if (m_id == 0)
		m_id = m_id_provider.allocate_id();
	std::string name = "!!Ntfctn" + std::to_string(m_id);
	int window_flags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;
	int child_window_flags = ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;
	if (imgui.begin(name, window_flags)) {
		ImGuiWindow* parent_window = ImGui::GetCurrentWindow();

		if (m_sp_state == SlicingProgressState::SP_CANCELLED || m_sp_state == SlicingProgressState::SP_COMPLETED) {
			ImVec2 button_size = ImVec2(38.f, 38.f) * scale;
			float  button_right_margin_x = 3.0f * scale;
			ImVec2 button_pos = window_pos + ImVec2(m_window_width - button_size.x - button_right_margin_x, (m_window_height - button_size.y) / 2.0f);
			float  text_left_margin_x = 15.0f * scale;
			ImVec2 text_pos = window_pos + ImVec2(text_left_margin_x, m_window_height / 2.0f - m_line_height * 1.2f);
			ImVec2 view_dailytips_text_pos = window_pos + ImVec2(text_left_margin_x, m_window_height / 2.0f + m_line_height * 0.2f);

			bbl_render_left_sign(imgui, m_window_width, m_window_height, window_pos.x + m_window_width, window_pos.y);
			render_text(text_pos);
			render_close_button(button_pos, button_size);
			render_show_dailytips(view_dailytips_text_pos);
		}

		if (m_sp_state == SlicingProgressState::SP_PROGRESS || m_sp_state == SlicingProgressState::SP_BEFORE_COMPLETED) {
			std::string child_name = "##SlicingProgressPanel" + std::to_string(parent_window->ID);

			ImGui::SetNextWindowPos(parent_window->Pos + progress_child_window_padding);
			if (ImGui::BeginChild(child_name.c_str(), ImVec2(progress_panel_width, progress_panel_height), false, child_window_flags)) {
				ImVec2 child_window_pos = ImGui::GetWindowPos();
				ImVec2 button_size = ImVec2(38.f, 38.f) * scale;
				ImVec2 button_pos = child_window_pos + ImVec2(progress_panel_width - button_size.x, (progress_panel_height - button_size.y) / 2.0f);
				float  margin_x = 8.0f * scale;
				ImVec2 progress_bar_pos = child_window_pos + ImVec2(0, progress_panel_height / 2.0f);
				ImVec2 progress_bar_size = ImVec2(progress_panel_width - button_size.x - margin_x, 4.0f * scale);
				ImVec2 text_pos = ImVec2(progress_bar_pos.x, progress_bar_pos.y - m_line_height * 1.2f);

				render_text(text_pos);
				if (m_sp_state == SlicingProgressState::SP_PROGRESS) {
					render_bar(progress_bar_pos, progress_bar_size);
					render_cancel_button(button_pos, button_size);
				}
			}
			ImGui::EndChild();

			// Separator Line
			ImVec2 separator_min = ImVec2(ImGui::GetCursorScreenPos().x + progress_child_window_padding.x, ImGui::GetCursorScreenPos().y);
			ImVec2 separator_max = ImVec2(ImGui::GetCursorScreenPos().x + progress_child_window_padding.x + progress_panel_width, ImGui::GetCursorScreenPos().y);
			ImGui::GetCurrentWindow()->DrawList->AddLine(separator_min, separator_max, ImColor(238, 238, 238));

			child_name = "##DailyTipsPanel" + std::to_string(parent_window->ID);
			ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos() + dailytips_child_window_padding);
			if (ImGui::BeginChild(child_name.c_str(), ImVec2(dailytips_panel_width, dailytips_panel_height), false, child_window_flags)) {
				ImVec2 child_window_pos = ImGui::GetWindowPos();
				ImVec2 dailytips_pos = child_window_pos;
				ImVec2 dailytips_size = ImVec2(dailytips_panel_width, dailytips_panel_height);
				render_dailytips_panel(dailytips_pos, dailytips_size);
			}
			ImGui::EndChild();
		}

		if (ImGui::IsMouseHoveringRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize(), true)) {
			set_hovered();
		}
	}
	imgui.end();


	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(1);
	restore_default_theme();
	if (fading_pop)
		ImGui::PopStyleColor(3);
}

void Slic3r::GUI::NotificationManager::SlicingProgressNotification::render_text(const ImVec2& pos)
{
	ImGuiWrapper& imgui = *wxGetApp().imgui();
	float scale = imgui.get_font_size() / 15.0f;
	if (m_sp_state == SlicingProgressState::SP_BEFORE_COMPLETED) {
		// complete icon
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
		push_style_color(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f), m_state == EState::FadingOut, m_current_fade_opacity);
		push_style_color(ImGuiCol_TextSelectedBg, ImVec4(0, .75f, .75f, 1.f), m_state == EState::FadingOut, m_current_fade_opacity);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));

		ImVec2 icon_size = ImVec2(38.f, 38.f) * scale;
		ImGui::SetCursorScreenPos(pos);
		std::wstring icon_text;
		icon_text = ImGui::CompleteIcon;
		imgui.button(icon_text.c_str());

		ImGui::PopStyleColor(5);

		// complete text
		imgui.push_bold_font();
		ImGui::SetCursorScreenPos(ImVec2(pos.x + icon_size.x, pos.y));
		imgui.text(m_text1.substr(0, m_endlines[0]).c_str());
		imgui.pop_bold_font();

		// before closing text
		int64_t now = GLCanvas3D::timestamp_now();
		int64_t duration_time = now - m_before_complete_start;
		if (duration_time > BEFORE_COMPLETE_DURATION) {
			set_progress_state(SlicingProgressState::SP_COMPLETED);
			return;
		}
		boost::format fmt(_u8L("Closing in %ds"));
		fmt % (3 - duration_time / 1000);
		ImGui::PushStyleColor(ImGuiCol_Text, ImColor(144, 144, 144).Value);
		ImGui::SetCursorScreenPos(ImVec2(pos.x + icon_size.x, pos.y + m_line_height + m_line_height / 2));
		imgui.text(fmt.str());
		ImGui::PopStyleColor();

	}
	else {
		//one line text
		ImGui::SetCursorScreenPos(pos);
		imgui.text(m_text1.substr(0, m_endlines[0]).c_str());
	}
}

void Slic3r::GUI::NotificationManager::SlicingProgressNotification::render_bar(const ImVec2& pos, const ImVec2& size)
{
	if (m_sp_state != SlicingProgressState::SP_PROGRESS)
		return;

	ImGuiWrapper& imgui = *wxGetApp().imgui();

	ImColor progress_color = ImColor(0, 174, 66, (int)(255 * m_current_fade_opacity));
	ImColor bg_color = ImColor(217, 217, 217, (int)(255 * m_current_fade_opacity));

	ImVec2 lineStart = pos;
	ImVec2 lineEnd = lineStart + size;
	ImVec2 midPoint = ImVec2(lineStart.x + (lineEnd.x - lineStart.x) * m_percentage, lineEnd.y);
	ImGui::GetWindowDrawList()->AddRectFilled(lineStart, lineEnd, bg_color);
	ImGui::GetWindowDrawList()->AddRectFilled(lineStart, midPoint, progress_color);
	
	// percentage text
	ImVec2 text_pos = ImVec2(pos.x, pos.y + size.y + m_line_height * 0.2f);
	std::string text;
	std::stringstream stream;
	stream << std::fixed << std::setprecision(2) << (int)(m_percentage * 100) << "%";
	text = stream.str();
	ImGui::SetCursorScreenPos(text_pos);
	imgui.text(text.c_str());
}

void NotificationManager::SlicingProgressNotification::render_dailytips_panel(const ImVec2& pos, const ImVec2& size)
{
	if (m_sp_state == SlicingProgressState::SP_BEFORE_COMPLETED)
		m_dailytips_panel->set_can_expand(false);
	else
		m_dailytips_panel->set_can_expand(true);
	m_dailytips_panel->set_position(pos);
	m_dailytips_panel->set_size(size);
	m_dailytips_panel->render();
}

void NotificationManager::SlicingProgressNotification::render_show_dailytips(const ImVec2& pos)
{
	if (m_sp_state != SlicingProgressState::SP_COMPLETED && m_sp_state != SlicingProgressState::SP_CANCELLED)
		return;

	ImGuiWrapper& imgui = *wxGetApp().imgui();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImColor(31, 142, 234).Value);

	ImGui::SetCursorScreenPos(pos);
	std::wstring button_text;
	button_text = ImGui::OpenArrowIcon;
	imgui.button((_u8L("View all Daily tips") + " " + button_text).c_str());
	//click behavior
	if (ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), true))
	{
		//underline
		ImVec2 lineEnd = ImGui::GetItemRectMax();
		lineEnd.x -= ImGui::CalcTextSize("A").x / 2;
		lineEnd.y -= 2;
		ImVec2 lineStart = lineEnd;
		lineStart.x = ImGui::GetItemRectMin().x;
		ImGui::GetWindowDrawList()->AddLine(lineStart, lineEnd, ImColor(31, 142, 234));

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			on_show_dailytips();
	}

	ImGui::PopStyleColor(4);
}

void NotificationManager::SlicingProgressNotification::on_show_dailytips()
{
	wxGetApp().plater()->get_dailytips()->open();
}

void Slic3r::GUI::NotificationManager::SlicingProgressNotification::render_cancel_button(const ImVec2& pos, const ImVec2& size)
{
	if (m_sp_state == SlicingProgressState::SP_PROGRESS) {
		ImGuiWrapper& imgui = *wxGetApp().imgui();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
		push_style_color(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f), m_state == EState::FadingOut, m_current_fade_opacity);
		push_style_color(ImGuiCol_TextSelectedBg, ImVec4(0, .75f, .75f, 1.f), m_state == EState::FadingOut, m_current_fade_opacity);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));

		ImVec2 button_size = size;
		ImVec2 button_pos = pos;
		ImGui::SetCursorScreenPos(button_pos);

		std::wstring button_text;
		button_text = ImGui::CancelButton;
		if (ImGui::IsMouseHoveringRect(button_pos, button_pos + button_size, true))
		{
			button_text = ImGui::CancelHoverButton;
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				on_cancel_button();
		}
		imgui.button(button_text.c_str());

		ImGui::PopStyleColor(5);
	}
}

void NotificationManager::SlicingProgressNotification::render_close_button(const ImVec2& pos, const ImVec2& size)
{
	if (m_sp_state == SlicingProgressState::SP_CANCELLED || m_sp_state == SlicingProgressState::SP_COMPLETED) {
		ImGuiWrapper& imgui = *wxGetApp().imgui();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.0f, .0f, .0f, .0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.0f, .0f, .0f, .0f));
		push_style_color(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f), m_state == EState::FadingOut, m_current_fade_opacity);
		push_style_color(ImGuiCol_TextSelectedBg, ImVec4(0, .75f, .75f, 1.f), m_state == EState::FadingOut, m_current_fade_opacity);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(.0f, .0f, .0f, .0f));

		ImVec2 button_size = size;
		ImVec2 button_pos = pos;
		ImGui::SetCursorScreenPos(button_pos);

		std::wstring button_text;
		button_text = m_is_dark ? ImGui::CloseNotifDarkButton : ImGui::CloseNotifButton;
		if (ImGui::IsMouseHoveringRect(button_pos, button_pos + button_size, true))
		{
			button_text = m_is_dark ? ImGui::CloseNotifHoverDarkButton : ImGui::CloseNotifHoverButton;
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				close();
		}
		imgui.button(button_text.c_str());

		ImGui::PopStyleColor(5);
	}
}

}}