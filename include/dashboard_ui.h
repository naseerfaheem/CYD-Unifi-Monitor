#ifndef DASHBOARD_UI_H
#define DASHBOARD_UI_H

#include <lvgl.h>
#include "config.h"
#include "unifi_api.h"

class DashboardUI {
private:
    // Main screen objects
    lv_obj_t* screen;
    lv_obj_t* header;
    lv_obj_t* chart_container;
    lv_obj_t* chart;
    lv_obj_t* speed_container;
    lv_obj_t* stats_container;
    lv_obj_t* message_label;
    
    // Labels
    lv_obj_t* title_label;
    lv_obj_t* time_label;
    lv_obj_t* download_label;
    lv_obj_t* upload_label;
    lv_obj_t* devices_label;
    lv_obj_t* ping_label;
    lv_obj_t* data_today_label;
    lv_obj_t* packet_loss_label;
    lv_obj_t* uptime_label;
    lv_obj_t* wan_status_label;
    
    // Chart series
    lv_chart_series_t* download_series;
    lv_chart_series_t* upload_series;
    
    // Chart data buffers - use smaller type to save RAM
    uint16_t download_history[CHART_HISTORY_POINTS];
    uint16_t upload_history[CHART_HISTORY_POINTS];
    int history_index;
    
    // Styling
    lv_style_t style_background;
    lv_style_t style_container;
    lv_style_t style_header;
    lv_style_t style_text_large;
    lv_style_t style_text_small;
    
    // Helper methods
    void create_styles();
    void create_header();
    void create_chart();
    void create_speed_display();
    void create_stats_display();
    String format_bytes(unsigned long bytes);
    String format_uptime(unsigned long seconds);
    lv_color_t get_status_color(String status);
    
public:
    DashboardUI();
    ~DashboardUI();
    
    // Initialization
    void init();
    
    // Update methods
    void updateStats(NetworkStats& stats);
    void addChartPoint(float download, float upload);
    void showMessage(String message);
    void updateTime();
    void refresh();
    
    // Utility
    void setTheme(bool dark);
    void clear();
};

#endif // DASHBOARD_UI_H