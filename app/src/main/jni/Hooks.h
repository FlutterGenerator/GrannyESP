#include "Canvas/ESP.h"
#include "Canvas/Bools.h"
#include "Canvas/StructsCommon.h"

#include "DarkManager.h"

#include "Substrate/SubstrateHook.h"
#include "Substrate/CydiaSubstrate.h"

//Target lib here
#define targetLibName OBFUSCATE("libil2cpp.so")

// ============================================================================
// RVAs taken from the ESP Tester report of com.dvloper.granny (32-bit, armeabi-v7a).
// They are valid for THAT game build only: after a game update, make a new report and replace them.
// ============================================================================
#define RVA_ENEMY_TICK     0xA3D66C   // EnemyAIGranny.FixedUpdate  (the class has no Update / OnDestroy)
#define RVA_CAMERA_MAIN    0x23CCD08  // Camera.get_main
#define RVA_CAMERA_CURRENT 0x23CCE98  // Camera.get_current
#define RVA_CAMERA_W2S     0x23CC994  // Camera.WorldToScreenPoint(Vector3)
#define RVA_GET_TRANSFORM  0x24049A8  // Component.get_transform
#define RVA_GET_POSITION   0x24174AC  // Transform.get_position
#define RVA_SCREEN_WIDTH   0x23D7D98  // Screen.get_width
#define RVA_SCREEN_HEIGHT  0x23D7DDC  // Screen.get_height

// Height of the enemy model in Unity units (metres). The box is drawn from the feet (the object's
// position) up to feet + this value. Make it bigger/smaller if the box is too tall/short.
#define ENEMY_HEIGHT 1.8f

// plain 3 floats: must be a trivial struct so it is passed/returned exactly like Unity's Vector3
struct UVec3 { float x, y, z; };

void *(*get_Transform)(void *instance, void *mi) = NULL;
UVec3 (*get_position)(void *transform, void *mi) = NULL;
void *(*get_camera)(void *mi) = NULL;
void *(*get_camera_current)(void *mi) = NULL;
UVec3 (*get_screenpos)(void *cam, UVec3 pos, void *mi) = NULL;
int (*get_screen_width)(void *mi) = NULL;
int (*get_screen_height)(void *mi) = NULL;

volatile bool espReady = false;

void (*old_Player_update)(void *, void *) = NULL;

// Runs on Unity's MAIN thread (it is the game's own FixedUpdate), so Unity calls are safe here.
// The screen position is computed here and only the finished numbers are stored.
void Player_update(void *player, void *mi) {
    // UnityEngine.Object.m_CachedPtr (2 pointers into the object) is 0 once Unity has destroyed it
    if (espReady && Esp && player != NULL &&
        *(void **) ((char *) player + 2 * sizeof(void *)) != NULL) {
        TrackedEnemy t;
        t.ok = false;
        t.headOk = false;
        t.hy = 0;
        t.sx = t.sy = t.sz = 0;
        t.seenMs = nowMs();

        static long long lastScreenMs = 0;
        if (get_screen_width && get_screen_height && t.seenMs - lastScreenMs > 500) {
            lastScreenMs = t.seenMs;
            unityScreenW = get_screen_width(NULL);
            unityScreenH = get_screen_height(NULL);
        }

        void *cam = get_camera ? get_camera(NULL) : NULL;
        if (cam == NULL && get_camera_current) cam = get_camera_current(NULL);
        void *tr = get_Transform(player, NULL);
        if (cam != NULL && tr != NULL) {
            UVec3 world = get_position(tr, NULL);
            UVec3 scr = get_screenpos(cam, world, NULL);
            t.sx = scr.x;
            t.sy = scr.y;
            t.sz = scr.z;
            t.ok = true;
            UVec3 head = world;
            head.y += ENEMY_HEIGHT;
            UVec3 hs = get_screenpos(cam, head, NULL);
            if (hs.z > 0) {
                t.hy = hs.y;
                t.headOk = true;
            }
        }
        std::lock_guard<std::mutex> g(enemiesMtx);
        enemies[player] = t;
    }
    if (old_Player_update) old_Player_update(player, mi);
}

// Runs on the overlay thread: only reads the stored numbers, never calls Unity.
void DrawESP(ESP esp, int screenWidth, int screenHeight) {

    esp.DrawText(Color::Red(), "tojik_proof_93 Esp Mod Menu", Vector2(screenWidth / 2, screenHeight / 1.02f), 25.0f);

    long long now = nowMs();

    if (EspObject) {
        int count = 0;
        {
            std::lock_guard<std::mutex> g(enemiesMtx);
            for (std::map<void *, TrackedEnemy>::iterator it = enemies.begin(); it != enemies.end(); ++it)
                if (now - it->second.seenMs <= 500) count++;
        }
        std::string Allplayers;
        Allplayers += "All Objects: ";
        Allplayers += std::to_string(count);

        esp.DrawText(Color::Black(), Allplayers.c_str(), Vector2(screenWidth / 2, screenHeight / 2), 35.0f);
    }

    if (Esp) {
        // Unity screen pixels -> overlay canvas pixels (they can differ)
        int uw = unityScreenW, uh = unityScreenH;
        float scaleX = uw > 0 ? (float) screenWidth / (float) uw : 1.0f;
        float scaleY = uh > 0 ? (float) screenHeight / (float) uh : 1.0f;

        std::lock_guard<std::mutex> g(enemiesMtx);
        for (std::map<void *, TrackedEnemy>::iterator it = enemies.begin(); it != enemies.end();) {
            const TrackedEnemy &t = it->second;
            if (now - t.seenMs > 2000) { // stopped updating: destroyed / scene changed
                it = enemies.erase(it);
                continue;
            }
            ++it;
            if (now - t.seenMs > 500 || !t.ok || t.sz <= 0) continue; // stale or behind the camera

            float x = t.sx * scaleX;
            float y = (float) screenHeight - t.sy * scaleY;
            if (x < -100 || x > screenWidth + 100 || y < -100 || y > screenHeight + 100) continue;

            // box from feet to head: size follows the model and the distance
            float boxH, boxW, top;
            if (t.headOk) {
                float headY = (float) screenHeight - t.hy * scaleY;
                top = headY < y ? headY : y;
                boxH = headY < y ? y - headY : headY - y;
                if (boxH < 8.0f) boxH = 8.0f;
                boxW = boxH * 0.55f;
            } else { // head not available: old fixed-size box around the point
                boxW = (float) screenHeight / 10.0f;
                boxH = (float) screenWidth / 16.0f;
                top = y - boxH / 2;
            }
            Rect PlayerRect(x - boxW / 2, top, boxW, boxH);
            float cy = top + boxH / 2; // line goes to the middle of the box

            if (EspLine) {
                esp.DrawLine(Color::White(), 1, Vector2(screenWidth * 0.5f, screenHeight * 0.5f), Vector2(x, cy));
            }

            if (EspBox) {
                esp.DrawBox(Color::White(), 1, PlayerRect);
            }
        }
    }
}

void hack_thread() {

    do {
        sleep(1);
    } while (!isLibraryLoaded(targetLibName));
    sleep(5); // let libil2cpp finish loading before patching it

    get_Transform = (void *(*)(void *, void *)) getAbsoluteAddress(targetLibName, RVA_GET_TRANSFORM);
    get_position = (UVec3 (*)(void *, void *)) getAbsoluteAddress(targetLibName, RVA_GET_POSITION);
    get_camera = (void *(*)(void *)) getAbsoluteAddress(targetLibName, RVA_CAMERA_MAIN);
    get_camera_current = (void *(*)(void *)) getAbsoluteAddress(targetLibName, RVA_CAMERA_CURRENT);
    get_screenpos = (UVec3 (*)(void *, UVec3, void *)) getAbsoluteAddress(targetLibName, RVA_CAMERA_W2S);
    get_screen_width = (int (*)(void *)) getAbsoluteAddress(targetLibName, RVA_SCREEN_WIDTH);
    get_screen_height = (int (*)(void *)) getAbsoluteAddress(targetLibName, RVA_SCREEN_HEIGHT);

    // public class EnemyAIGranny : MonoBehaviour { void FixedUpdate() }
    MSHookFunction((void *) getAbsoluteAddress(targetLibName, RVA_ENEMY_TICK),
                   (void *) &Player_update,
                   (void **) &old_Player_update);
    espReady = true;
}
