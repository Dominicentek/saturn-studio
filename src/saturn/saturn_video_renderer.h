#ifndef SaturnVideoRenderer
#define SaturnVideoRenderer

#include <tuple>
#include <utility>
#include <vector>
#include <string>

#define FUNC(args...) void(*args

#define FUNC_INIT       )(int w, int h, bool fps60)
#define FUNC_RENDER     )(unsigned char* data)
#define FUNC_FINALIZE   )()

typedef std::tuple<FUNC(FUNC_INIT), FUNC(FUNC_RENDER), FUNC(FUNC_FINALIZE), int> VideoRenderer;

extern FUNC(video_renderer_init      FUNC_INIT    );
extern FUNC(video_renderer_render    FUNC_RENDER  );
extern FUNC(video_renderer_finalize  FUNC_FINALIZE);
extern int  video_renderer_flags;

extern void saturn_set_video_destination(std::string path);
extern std::vector<std::string> video_renderer_get_formats(bool ffmpeg);

#define VIDEO_RENDERER_FLAGS_NONE        0
#define VIDEO_RENDERER_FLAGS_TRANSPARECY (1 << 0)
#define VIDEO_RENDERER_FLAGS_FFMPEG      (1 << 1)
#define VIDEO_RENDERER_FLAGS_60FPS       (1 << 2)

#endif
