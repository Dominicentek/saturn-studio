#include "saturn_video_renderer.h"

#include <cstring>
#include <iostream>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "saturn.h"

extern "C" {
#include "pc/pngutils.h"
}

void pngseq_finalize() {}

int video_width;
int video_height;
int png_counter;

FILE* ffmpeg;

void pngseq_init(int w, int h, bool fps60) {
    video_width = w;
    video_height = h;
    png_counter = 0;
    if (fs::exists("pngseq")) fs::remove_all("pngseq");
    fs::create_directory("pngseq");
}

void pngseq_render(unsigned char* data) {
    pngutils_write_png(("pngseq/" + std::to_string(++png_counter) + ".png").c_str(), video_width, video_height, 4, data, 0);
}

void webm_init(int w, int h, bool fps60) {
    video_width = w;
    video_height = h;
    std::string cmd = "ffmpeg -y -r " + std::string(fps60 ? "60" : "30") + " -f rawvideo -pix_fmt rgba -s " + std::to_string(w) + "x" + std::to_string(h) + " -i - -c:v libvpx-vp9 -pix_fmt yuva420p video.webm";
#ifdef _WIN32
    ffmpeg = popen(cmd.c_str(), "wb");
#else
    ffmpeg = popen(cmd.c_str(), "w");
#endif
}

void mp4_init(int w, int h, bool fps60) {
    video_width = w;
    video_height = h;
    std::string cmd = "ffmpeg -y -r " + std::string(fps60 ? "60" : "30") + " -f rawvideo -pix_fmt rgba -s " + std::to_string(w) + "x" + std::to_string(h) + " -i - -c:v h264 -pix_fmt yuv420p video.mp4";
#ifdef _WIN32
    ffmpeg = popen(cmd.c_str(), "wb");
#else
    ffmpeg = popen(cmd.c_str(), "w");
#endif
}

void gif_init(int w, int h, bool fps60) {
    video_width = w;
    video_height = h;
    std::string cmd = "ffmpeg -y -r 30 -f rawvideo -pix_fmt rgba -s " + std::to_string(w) + "x" + std::to_string(h) + " -i -  -vf \"format=rgba,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse\" video.gif";
#ifdef _WIN32
    ffmpeg = popen(cmd.c_str(), "wb");
#else
    ffmpeg = popen(cmd.c_str(), "w");
#endif
}

void mov_init(int w, int h, bool fps60) {
    video_width = w;
    video_height = h;
    std::string cmd = "ffmpeg -y -r " + std::string(fps60 ? "60" : "30") + " -f rawvideo -pix_fmt rgba -s " + std::to_string(w) + "x" + std::to_string(h) + " -i - -c:v qtrle -pix_fmt argb video.mov";
#ifdef _WIN32
    ffmpeg = popen(cmd.c_str(), "wb");
#else
    ffmpeg = popen(cmd.c_str(), "w");
#endif
}

void ffmpeg_render(unsigned char* data) {
    fwrite(data, video_width * video_height * 4, 1, ffmpeg);
}

void ffmpeg_finalize() {
    fclose(ffmpeg);
}

VideoRenderer renderer_pngseq = {
    pngseq_init,
    pngseq_render,
    pngseq_finalize,
    VIDEO_RENDERER_FLAGS_TRANSPARECY | VIDEO_RENDERER_FLAGS_60FPS,
};

VideoRenderer renderer_webm = {
    webm_init,
    ffmpeg_render,
    ffmpeg_finalize,
    VIDEO_RENDERER_FLAGS_FFMPEG | VIDEO_RENDERER_FLAGS_TRANSPARECY | VIDEO_RENDERER_FLAGS_60FPS,
};

VideoRenderer renderer_mp4 = {
    mp4_init,
    ffmpeg_render,
    ffmpeg_finalize,
    VIDEO_RENDERER_FLAGS_FFMPEG | VIDEO_RENDERER_FLAGS_60FPS,
};

VideoRenderer renderer_gif = {
    gif_init,
    ffmpeg_render,
    ffmpeg_finalize,
    VIDEO_RENDERER_FLAGS_FFMPEG | VIDEO_RENDERER_FLAGS_TRANSPARECY,
};

VideoRenderer renderer_mov = {
    mov_init,
    ffmpeg_render,
    ffmpeg_finalize,
    VIDEO_RENDERER_FLAGS_FFMPEG | VIDEO_RENDERER_FLAGS_TRANSPARECY | VIDEO_RENDERER_FLAGS_60FPS,
};

std::vector<std::pair<std::string, VideoRenderer>> video_renderers =  {
    { ".png sequence", renderer_pngseq },
    { ".webm",         renderer_webm   },
    { ".mp4",          renderer_mp4    },
    { ".gif",          renderer_gif    },
    { ".mov",          renderer_mov    },
};

FUNC(video_renderer_init      FUNC_INIT    ) = nullptr;
FUNC(video_renderer_render    FUNC_RENDER  ) = nullptr;
FUNC(video_renderer_finalize  FUNC_FINALIZE) = nullptr;
int  video_renderer_flags                    = VIDEO_RENDERER_FLAGS_NONE;

void saturn_set_video_renderer(int index) {
    VideoRenderer video_renderer = video_renderers[index].second;
    video_renderer_init     = std::get<0>(video_renderer);
    video_renderer_render   = std::get<1>(video_renderer);
    video_renderer_finalize = std::get<2>(video_renderer);
    video_renderer_flags    = std::get<3>(video_renderer);
}

std::vector<std::pair<int, std::string>> video_renderer_get_formats() {
    std::vector<std::pair<int, std::string>> formats = {};
    for (int i = 0; i < video_renderers.size(); i++) {
        formats.push_back({ std::get<3>(video_renderers[i].second), video_renderers[i].first });
    }
    return formats;
}
