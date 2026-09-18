// Milestone 61: collection burst restores immediate-mode state; missing sound
// fails safely. Hidden window. Not shipped.

#include "gameplay/ItemPickupCollectionFeedback.h"
#include "platform/GameplayAudio.h"
#include "render/ItemPickupCollectionFeedbackDraw.h"
#include "render/StaticModelScene.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* what)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++gFailures;
    }
}

constexpr unsigned int kGlBlend = 0x0BE2;
constexpr unsigned int kGlDepthTest = 0x0B71;
constexpr unsigned int kGlCullFace = 0x0B44;
constexpr unsigned int kGlDepthWritemask = 0x0B72;

using GlGetBooleanvFn = void (*)(unsigned int, unsigned char*);

struct Snapshot
{
    bool blend = false;
    bool depthTest = false;
    bool depthWriteMask = false;
    bool cullFace = false;
    float transform[16]{};
};

Snapshot QuerySnapshot()
{
    Snapshot state{};
    auto getBooleanv = reinterpret_cast<GlGetBooleanvFn>(rlGetProcAddress("glGetBooleanv"));
    if (getBooleanv != nullptr)
    {
        unsigned char blend = 0;
        unsigned char depth = 0;
        unsigned char cull = 0;
        unsigned char depthMask = 0;
        getBooleanv(kGlBlend, &blend);
        getBooleanv(kGlDepthTest, &depth);
        getBooleanv(kGlCullFace, &cull);
        getBooleanv(kGlDepthWritemask, &depthMask);
        state.blend = blend != 0;
        state.depthTest = depth != 0;
        state.cullFace = cull != 0;
        state.depthWriteMask = depthMask != 0;
    }
    const Matrix transform = rlGetMatrixTransform();
    state.transform[0] = transform.m0;
    state.transform[1] = transform.m5;
    state.transform[2] = transform.m10;
    state.transform[3] = transform.m12;
    state.transform[4] = transform.m13;
    state.transform[5] = transform.m14;
    state.transform[6] = transform.m15;
    return state;
}

bool SnapshotNear(const Snapshot& a, const Snapshot& b)
{
    return a.blend == b.blend && a.depthTest == b.depthTest && a.depthWriteMask == b.depthWriteMask
        && a.cullFace == b.cullFace
        && std::fabs(a.transform[0] - b.transform[0]) < 1.0e-4f
        && std::fabs(a.transform[1] - b.transform[1]) < 1.0e-4f
        && std::fabs(a.transform[2] - b.transform[2]) < 1.0e-4f
        && std::fabs(a.transform[3] - b.transform[3]) < 1.0e-4f
        && std::fabs(a.transform[4] - b.transform[4]) < 1.0e-4f
        && std::fabs(a.transform[5] - b.transform[5]) < 1.0e-4f;
}

Camera3D MakeCamera()
{
    Camera3D camera{};
    camera.position = Vector3{0.0f, 4.0f, 12.0f};
    camera.target = Vector3{0.0f, 1.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 40.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

void WritePcm16Wav(const std::filesystem::path& path)
{
    const int sampleRate = 22050;
    const int sampleCount = sampleRate / 20;
    const int dataBytes = sampleCount * 2;
    const int riffSize = 36 + dataBytes;
    std::ofstream out(path, std::ios::binary);
    const char riff[] = {'R', 'I', 'F', 'F'};
    const char wave[] = {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '};
    const char data[] = {'d', 'a', 't', 'a'};
    auto write32 = [&out](int value) {
        const unsigned int u = static_cast<unsigned int>(value);
        const char bytes[] = {
            static_cast<char>(u & 0xff),
            static_cast<char>((u >> 8) & 0xff),
            static_cast<char>((u >> 16) & 0xff),
            static_cast<char>((u >> 24) & 0xff)};
        out.write(bytes, 4);
    };
    auto write16 = [&out](int value) {
        const unsigned int u = static_cast<unsigned int>(value);
        const char bytes[] = {
            static_cast<char>(u & 0xff),
            static_cast<char>((u >> 8) & 0xff)};
        out.write(bytes, 2);
    };
    out.write(riff, 4);
    write32(riffSize);
    out.write(wave, 8);
    write32(16);
    write16(1);
    write16(1);
    write32(sampleRate);
    write32(sampleRate * 2);
    write16(2);
    write16(16);
    out.write(data, 4);
    write32(dataBytes);
    for (int index = 0; index < sampleCount; ++index)
    {
        write16(0);
    }
}
}

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(640, 360, "ItemPickupCollectionFeedbackDrawTest");

    platform::GameplayAudio sound;
    sound.PlayPickup();
    Expect(!sound.IsCueLoaded(platform::GameplaySfxCue::Pickup), "12. Play without load is safe");
    sound.Load();
    const int firstLoadAttempts = sound.LoadAttemptCount();
    sound.Load();
    Expect(
        firstLoadAttempts == 1 && sound.LoadAttemptCount() == 1,
        "14. second Load does not reload fixed gameplay sounds");
    sound.PlayPickup();
    sound.PlayDamage();
    sound.PlayDeath();
    sound.PlayRespawn();
    sound.PlayFootstep();
    sound.PlayJump();
    sound.PlayLanding();
    sound.Unload();
    Expect(!sound.IsCueLoaded(platform::GameplaySfxCue::Pickup), "unload after staged Load is safe");
    sound.LoadCueFromPath(platform::GameplaySfxCue::Pickup, std::filesystem::path{});
    Expect(!sound.IsCueLoaded(platform::GameplaySfxCue::Pickup), "12. empty path fails safely");
    sound.PlayPickup();
    const std::filesystem::path missing =
        std::filesystem::temp_directory_path() / "platformer_missing_pickup_collect.wav";
    std::filesystem::remove(missing);
    sound.LoadCueFromPath(platform::GameplaySfxCue::Pickup, missing);
    Expect(!sound.IsCueLoaded(platform::GameplaySfxCue::Pickup), "12. missing sound fails safely");
    sound.PlayPickup();

    const std::filesystem::path valid =
        std::filesystem::temp_directory_path() / "platformer_test_pickup_collect.wav";
    WritePcm16Wav(valid);
    sound.LoadCueFromPath(platform::GameplaySfxCue::Pickup, valid);
    Expect(sound.IsCueLoaded(platform::GameplaySfxCue::Pickup), "11. valid short WAV loads once");
    sound.PlayPickup();
    sound.PlayPickup();
    Expect(sound.IsCueLoaded(platform::GameplaySfxCue::Pickup), "11. repeated Play keeps the cached sound");
    sound.LoadCueFromPath(
        platform::GameplaySfxCue::Damage,
        std::filesystem::temp_directory_path() / "platformer_missing_damage.wav");
    Expect(
        sound.IsCueLoaded(platform::GameplaySfxCue::Pickup)
            && !sound.IsCueLoaded(platform::GameplaySfxCue::Damage),
        "13. missing individual cue leaves other loaded sounds and is a no-op");
    sound.PlayDamage();
    sound.LoadCueFromPath(
        platform::GameplaySfxCue::Jump,
        std::filesystem::temp_directory_path() / "platformer_missing_jump.wav");
    Expect(
        sound.IsCueLoaded(platform::GameplaySfxCue::Pickup)
            && !sound.IsCueLoaded(platform::GameplaySfxCue::Jump),
        "19. missing individual movement cue leaves other loaded sounds and is a no-op");
    sound.PlayJump();
    sound.Unload();
    Expect(!sound.IsCueLoaded(platform::GameplaySfxCue::Pickup), "unload clears the cached sound");
    std::filesystem::remove(valid);

    gameplay::ItemPickupCollectionFeedbackState feedback{};
    world::ItemPickupSpec pickup{};
    pickup.position = {1.0f, 1.0f, 0.0f};
    pickup.itemId = "key";
    Expect(gameplay::SpawnItemPickupCollectionFeedback(feedback, pickup, 0.0), "spawn for draw");

    const Camera3D camera = MakeCamera();
    BeginDrawing();
    ClearBackground(Color{32, 36, 48, 255});
    BeginMode3D(camera);
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    render::RestoreGreyboxImmediateState();
    DrawCube(Vector3{0.0f, 0.0f, 0.0f}, 2.0f, 0.4f, 2.0f, Color{78, 84, 96, 255});
    const Snapshot before = QuerySnapshot();
    render::DrawItemPickupCollectionFeedback(feedback);
    const Snapshot after = QuerySnapshot();
    Expect(SnapshotNear(before, after), "20. collection burst restores renderer state");
    Expect(before.depthTest && after.depthTest, "20. depth test restored");
    Expect(before.depthWriteMask && after.depthWriteMask, "20. depth mask restored");
    Expect(before.cullFace && after.cullFace, "20. culling restored");
    DrawCube(Vector3{2.0f, 1.0f, 0.0f}, 0.5f, 0.5f, 0.5f, Color{216, 96, 72, 255});
    EndMode3D();
    EndDrawing();

    gameplay::ItemPickupCollectionFeedbackState empty{};
    BeginDrawing();
    ClearBackground(Color{32, 36, 48, 255});
    BeginMode3D(camera);
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    render::RestoreGreyboxImmediateState();
    const Snapshot emptyBefore = QuerySnapshot();
    render::DrawItemPickupCollectionFeedback(empty);
    const Snapshot emptyAfter = QuerySnapshot();
    Expect(SnapshotNear(emptyBefore, emptyAfter), "empty feedback does not disturb renderer state");
    EndMode3D();
    EndDrawing();

    CloseWindow();

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d ItemPickupCollectionFeedbackDrawTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("ItemPickupCollectionFeedbackDrawTest passed\n");
    return 0;
}
