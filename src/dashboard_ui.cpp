#include "dashboard_ui.h"
#include "config.h"
#include <time.h>

DashboardUI::DashboardUI() : history_index(0) {
    // Initialize history buffers
    for (int i = 0; i < CHART_HISTORY_POINTS; i++) {
        download_history[i] = 0;
        upload_history[i] = 0;
    }
}

DashboardUI::~DashboardUI() {
    // Cleanup handled by LVGL
}

void DashboardUI::init() {
    // Create styles
    create_styles();
    
    // Create main screen
    screen = lv_scr_act();
    lv_obj_add_style(screen, &style_background, 0);
    
    // Create UI components
    create_header();
    create_chart();
    create_speed_display();
    create_stats_display();
    
    // Create message label (hidden by default)
    message_label = lv_label_create(screen);
    lv_obj_set_size(message_label, SCREEN_WIDTH - 20, 40);
    lv_obj_align(message_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(message_label, "");
    lv_obj_add_flag(message_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_CENTER, 0);
}

void DashboardUI::create_styles() {
    // Background style
    lv_style_init(&style_background);
    lv_style_set_bg_color(&style_background, lv_color_hex(COLOR_BACKGROUND));
    lv_style_set_bg_opa(&style_background, LV_OPA_COVER);
    
    // Container style
    lv_style_init(&style_container);
    lv_style_set_bg_color(&style_container, lv_color_hex(0x1a1a1a));
    lv_style_set_bg_opa(&style_container, LV_OPA_90);
    lv_style_set_radius(&style_container, 8);
    lv_style_set_border_width(&style_container, 1);
    lv_style_set_border_color(&style_container, lv_color_hex(0x333333));
    lv_style_set_pad_all(&style_container, 8);
    
    // Header style
    lv_style_init(&style_header);
    lv_style_set_bg_color(&style_header, lv_color_hex(0x0a0a0a));
    lv_style_set_bg_opa(&style_header, LV_OPA_COVER);
    lv_style_set_border_width(&style_header, 0);
    lv_style_set_pad_all(&style_header, 10);
    
    // Large text style
    lv_style_init(&style_text_large);
    lv_style_set_text_color(&style_text_large, lv_color_hex(COLOR_TEXT));
    lv_style_set_text_font(&style_text_large, &lv_font_montserrat_24);
    
    // Small text style
    lv_style_init(&style_text_small);
    lv_style_set_text_color(&style_text_small, lv_color_hex(COLOR_TEXT_DIM));
    lv_style_set_text_font(&style_text_small, &lv_font_montserrat_12);
}

void DashboardUI::create_header() {
    // Header container
    header = lv_obj_create(screen);
    lv_obj_set_size(header, SCREEN_WIDTH, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(header, &style_header, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    title_label = lv_label_create(header);
    lv_label_set_text(title_label, "UniFi Network Monitor");
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(COLOR_PRIMARY), 0);
    
    // Time
    time_label = lv_label_create(header);
    lv_label_set_text(time_label, "00:00");
    lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, -10, 0);
}

void DashboardUI::create_chart() {
    // Chart container
    chart_container = lv_obj_create(screen);
    lv_obj_set_size(chart_container, SCREEN_WIDTH - 20, 180);
    lv_obj_align(chart_container, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_add_style(chart_container, &style_container, 0);
    lv_obj_clear_flag(chart_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create chart
    chart = lv_chart_create(chart_container);
    lv_obj_set_size(chart, SCREEN_WIDTH - 40, 160);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);
    
    // Configure chart
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, CHART_HISTORY_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(chart, CHART_Y_GRID_COUNT, 0);
    
    // Style the chart
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
    
    // Add series
    download_series = lv_chart_add_series(chart, lv_color_hex(COLOR_PRIMARY), LV_CHART_AXIS_PRIMARY_Y);
    upload_series = lv_chart_add_series(chart, lv_color_hex(COLOR_SECONDARY), LV_CHART_AXIS_PRIMARY_Y);
    
    // Set line width
    lv_chart_set_series_color(chart, download_series, lv_color_hex(COLOR_PRIMARY));
    lv_chart_set_series_color(chart, upload_series, lv_color_hex(COLOR_SECONDARY));
}

void DashboardUI::create_speed_display() {
    // Speed container
    speed_container = lv_obj_create(screen);
    lv_obj_set_size(speed_container, SCREEN_WIDTH - 20, 60);
    lv_obj_align(speed_container, LV_ALIGN_TOP_MID, 0, 240);
    lv_obj_add_style(speed_container, &style_container, 0);
    lv_obj_clear_flag(speed_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Download speed
    lv_obj_t* down_container = lv_obj_create(speed_container);
    lv_obj_set_size(down_container, (SCREEN_WIDTH - 60) / 2, 44);
    lv_obj_align(down_container, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(down_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(down_container, 0, 0);
    lv_obj_set_style_pad_all(down_container, 0, 0);
    lv_obj_clear_flag(down_container, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* down_icon = lv_label_create(down_container);
    lv_label_set_text(down_icon, LV_SYMBOL_DOWNLOAD);
    lv_obj_align(down_icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(down_icon, lv_color_hex(COLOR_PRIMARY), 0);
    
    download_label = lv_label_create(down_container);
    lv_label_set_text(download_label, "0.0 Mbps");
    lv_obj_align(download_label, LV_ALIGN_CENTER, 10, 0);
    lv_obj_add_style(download_label, &style_text_large, 0);
    
    // Upload speed
    lv_obj_t* up_container = lv_obj_create(speed_container);
    lv_obj_set_size(up_container, (SCREEN_WIDTH - 60) / 2, 44);
    lv_obj_align(up_container, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(up_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(up_container, 0, 0);
    lv_obj_set_style_pad_all(up_container, 0, 0);
    lv_obj_clear_flag(up_container, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* up_icon = lv_label_create(up_container);
    lv_label_set_text(up_icon, LV_SYMBOL_UPLOAD);
    lv_obj_align(up_icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(up_icon, lv_color_hex(COLOR_SECONDARY), 0);
    
    upload_label = lv_label_create(up_container);
    lv_label_set_text(upload_label, "0.0 Mbps");
    lv_obj_align(upload_label, LV_ALIGN_CENTER, 10, 0);
    lv_obj_add_style(upload_label, &style_text_large, 0);
}

void DashboardUI::create_stats_display() {
    // Stats container
    stats_container = lv_obj_create(screen);
    lv_obj_set_size(stats_container, SCREEN_WIDTH - 20, 160);
    lv_obj_align(stats_container, LV_ALIGN_TOP_MID, 0, 310);
    lv_obj_add_style(stats_container, &style_container, 0);
    lv_obj_clear_flag(stats_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Create 3x3 grid of stats
    int y_positions[] = {10, 50, 90};
    int x_positions[] = {10, (SCREEN_WIDTH - 20) / 2};
    
    // Row 1: Devices and Ping
    devices_label = lv_label_create(stats_container);
    lv_label_set_text(devices_label, "Devices: 0");
    lv_obj_set_pos(devices_label, x_positions[0], y_positions[0]);
    
    ping_label = lv_label_create(stats_container);
    lv_label_set_text(ping_label, "Ping: -- ms");
    lv_obj_set_pos(ping_label, x_positions[1], y_positions[0]);
    
    // Row 2: Today's data and Packet loss
    data_today_label = lv_label_create(stats_container);
    lv_label_set_text(data_today_label, "Today: 0 GB");
    lv_obj_set_pos(data_today_label, x_positions[0], y_positions[1]);
    
    packet_loss_label = lv_label_create(stats_container);
    lv_label_set_text(packet_loss_label, "Loss: 0.0%");
    lv_obj_set_pos(packet_loss_label, x_positions[1], y_positions[1]);
    
    // Row 3: Uptime and WAN status
    uptime_label = lv_label_create(stats_container);
    lv_label_set_text(uptime_label, "Uptime: --");
    lv_obj_set_pos(uptime_label, x_positions[0], y_positions[2]);
    
    wan_status_label = lv_label_create(stats_container);
    lv_label_set_text(wan_status_label, "WAN: Unknown");
    lv_obj_set_pos(wan_status_label, x_positions[1], y_positions[2]);
}

void DashboardUI::updateStats(NetworkStats& stats) {
    // Update speed labels
    char buffer[32];
    
    snprintf(buffer, sizeof(buffer), "%.1f Mbps", stats.download_mbps);
    lv_label_set_text(download_label, buffer);
    
    snprintf(buffer, sizeof(buffer), "%.1f Mbps", stats.upload_mbps);
    lv_label_set_text(upload_label, buffer);
    
    // Update device count
    snprintf(buffer, sizeof(buffer), "Devices: %d", stats.connected_devices);
    lv_label_set_text(devices_label, buffer);
    
    // Update ping
    if (stats.ping_ms > 0) {
        snprintf(buffer, sizeof(buffer), "Ping: %.0f ms", stats.ping_ms);
        lv_label_set_text(ping_label, buffer);
        
        // Color code ping
        if (stats.ping_ms < 30) {
            lv_obj_set_style_text_color(ping_label, lv_color_hex(COLOR_SUCCESS), 0);
        } else if (stats.ping_ms < 100) {
            lv_obj_set_style_text_color(ping_label, lv_color_hex(COLOR_WARNING), 0);
        } else {
            lv_obj_set_style_text_color(ping_label, lv_color_hex(COLOR_ERROR), 0);
        }
    }
    
    // Update data usage
    String todayStr = "Today: " + format_bytes(stats.total_bytes_today);
    lv_label_set_text(data_today_label, todayStr.c_str());
    
    // Update packet loss
    snprintf(buffer, sizeof(buffer), "Loss: %.1f%%", stats.packet_loss);
    lv_label_set_text(packet_loss_label, buffer);
    
    // Color code packet loss
    if (stats.packet_loss < 0.1) {
        lv_obj_set_style_text_color(packet_loss_label, lv_color_hex(COLOR_SUCCESS), 0);
    } else if (stats.packet_loss < 1.0) {
        lv_obj_set_style_text_color(packet_loss_label, lv_color_hex(COLOR_WARNING), 0);
    } else {
        lv_obj_set_style_text_color(packet_loss_label, lv_color_hex(COLOR_ERROR), 0);
    }
    
    // Update uptime
    String uptimeStr = "Uptime: " + format_uptime(stats.uptime_seconds);
    lv_label_set_text(uptime_label, uptimeStr.c_str());
    
    // Update WAN status
    String wanStr = "WAN: " + stats.wan_status;
    lv_label_set_text(wan_status_label, wanStr.c_str());
    lv_obj_set_style_text_color(wan_status_label, get_status_color(stats.wan_status), 0);
}

void DashboardUI::addChartPoint(float download, float upload) {
    // Add to history buffers (convert to uint16_t to save RAM)
    download_history[history_index] = (uint16_t)download;
    upload_history[history_index] = (uint16_t)upload;
    
    // Update chart data
    lv_chart_set_next_value(chart, download_series, (int)download);
    lv_chart_set_next_value(chart, upload_series, (int)upload);
    
    // Auto-scale if needed
    if (CHART_AUTO_SCALE) {
        float max_value = download;
        if (upload > max_value) max_value = upload;
        
        // Find max in history
        for (int i = 0; i < CHART_HISTORY_POINTS; i++) {
            if (download_history[i] > max_value) max_value = download_history[i];
            if (upload_history[i] > max_value) max_value = upload_history[i];
        }
        
        // Set appropriate scale
        int scale = 10;
        if (max_value > 500) scale = 1000;
        else if (max_value > 200) scale = 500;
        else if (max_value > 100) scale = 200;
        else if (max_value > 50) scale = 100;
        else if (max_value > 20) scale = 50;
        
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, scale);
    }
    
    // Update history index
    history_index = (history_index + 1) % CHART_HISTORY_POINTS;
    
    // Refresh chart
    lv_chart_refresh(chart);
}

void DashboardUI::showMessage(String message) {
    if (message.length() > 0) {
        lv_label_set_text(message_label, message.c_str());
        lv_obj_clear_flag(message_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(message_label, LV_OBJ_FLAG_HIDDEN);
    }
}

void DashboardUI::updateTime() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
        lv_label_set_text(time_label, timeStr);
    }
}

void DashboardUI::refresh() {
    updateTime();
    lv_refr_now(NULL);
}

String DashboardUI::format_bytes(unsigned long bytes) {
    if (bytes < 1024) {
        return String(bytes) + " B";
    } else if (bytes < 1048576) {
        return String(bytes / 1024.0, 1) + " KB";
    } else if (bytes < 1073741824) {
        return String(bytes / 1048576.0, 1) + " MB";
    } else {
        return String(bytes / 1073741824.0, 2) + " GB";
    }
}

String DashboardUI::format_uptime(unsigned long seconds) {
    if (seconds == 0) return "--";
    
    unsigned long days = seconds / 86400;
    unsigned long hours = (seconds % 86400) / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    
    if (days > 0) {
        return String(days) + "d " + String(hours) + "h";
    } else if (hours > 0) {
        return String(hours) + "h " + String(minutes) + "m";
    } else {
        return String(minutes) + "m";
    }
}

lv_color_t DashboardUI::get_status_color(String status) {
    if (status == "OK" || status == "Connected") {
        return lv_color_hex(COLOR_SUCCESS);
    } else if (status == "WARNING") {
        return lv_color_hex(COLOR_WARNING);
    } else {
        return lv_color_hex(COLOR_ERROR);
    }
}