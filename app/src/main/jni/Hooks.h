#include "Canvas/ESP.h"
#include "Canvas/Bools.h"
#include "Canvas/StructsCommon.h"

#include "DarkManager.h"

#include "AutoHook/AutoHook.h"

#include "And64InlineHook/And64InlineHook.hpp"

#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

//Target lib here
#define targetLibName OBFUSCATE("libil2cpp.so")

void *(*get_Transform)(void *instance);
Vector3 (*get_position)(void *instance);
void *(*get_camera)();
Vector3 (*get_screenpos)(void *cam, Vector3 pos);

Vector3 getPosition(void *transform) {
    return get_position(get_Transform(transform));
} 

void (*old_Player_update)(...);
void Player_update(void *player) {
    if (player != NULL) {
        
        if (!playerFind(player)) players.push_back(player);
        if (players.size() > 15) {
            players.clear();
        }
    }
    clearPlayers();
    old_Player_update(player);
}

void DrawESP(ESP esp, int screenWidth, int screenHeight) {
    
    esp.DrawText(Color::Red(), "tojik_proof_93 Esp Mod Menu", Vector2(screenWidth / 2, screenHeight / 1.02f), 25.0f); 
           
    if(EspObject) {
         
         std::string Allplayers;     
         Allplayers += "All Objects: ";
         Allplayers += std::to_string((int32_t) players.size());
  
         esp.DrawText(Color::Black(), Allplayers.c_str(), Vector2(screenWidth / 2 ,screenHeight / 2), 35.0f);      
      }  
    
  if(Esp) {  
  
   // auto Screen_SetResolution = (void (*)(int, int, bool)) (getAbsoluteAddress("libil2cpp.so",0x176DDAC));
   // Screen_SetResolution(screenWidth, screenHeight, true);
      
    for (int i = 0; i < players.size(); i++) {
       
        void *Player;   
        
        if (i < players.size()) Player = players[i];
        
        if (Player != NULL && get_camera() != NULL) {  
                                             
            auto isPlayerLocation = get_screenpos(get_camera(),getPosition(Player));
              
            float isBoxWidth = (float) screenWidth / 16.0f;
            float isBoxHeight = (float) screenHeight / 10.0f;

            Vector2 isLocator = Vector2(isPlayerLocation.x, screenHeight - isPlayerLocation.y);
            Rect PlayerRect(isLocator.X - (isBoxHeight / 2), isLocator.Y - (isBoxWidth / 2), isBoxHeight, isBoxWidth);
         
            if (EspLine) {                 
                esp.DrawLine(Color::White(), 1,Vector2(screenWidth * 0.5f, screenHeight * 0.5f),Vector2(isPlayerLocation.x,screenHeight - isPlayerLocation.y)); 
             }
             
            if (EspBox) {         
                 esp.DrawBox(Color::White(), 1, PlayerRect);            
               }             
            }                    
        }
    }
}

void hack_thread() {
    
     do {
        sleep(1);
    } while (!isLibraryLoaded(targetLibName));
    
    // ============================
    // AUTO IL2CPP OFFSETS
    // ============================

    // Component.get_transform()
    auto ComponentClass = new LoadClass("UnityEngine", "Component");
    get_Transform = (void *(*)(void *))
        ComponentClass->GetMethodOffsetByName("get_transform", 0);

    // Transform.get_position()
    auto TransformClass = new LoadClass("UnityEngine", "Transform");
    get_position = (Vector3 (*)(void*))
        TransformClass->GetMethodOffsetByName("get_position", 0);

    // Camera.get_main()
    auto CameraClass = new LoadClass("UnityEngine", "Camera");
    get_camera = (void *(*)())
        CameraClass->GetMethodOffsetByName("get_main", 0);

    // Camera.WorldToScreenPoint(Vector3)
    get_screenpos = (Vector3 (*)(void *, Vector3))
        CameraClass->GetMethodOffsetByName("WorldToScreenPoint", 1);

    // Player.Update()
    auto PlayerClass = new LoadClass("", "EnemyAIGranny");
    DWORD PlayerUpdateOffset = PlayerClass->GetMethodOffsetByName("FixedUpdate", 0);

// ============================
    // HOOK (choose one)
    // ============================

#if defined(__aarch64__)
    A64HookFunction((void *) PlayerUpdateOffset,
                    (void *) Player_update,
                    (void **) &old_Player_update);
#else
    MSHookFunction((void *) PlayerUpdateOffset,
                   (void *) Player_update,
                   (void **) &old_Player_update);
#endif

    // //Component.get_transform();
    // get_Transform = (void *(*)(void *)) getAbsoluteAddress(targetLibName, 0x1739554);
    
    // //Transform.get_position();
    // get_position = (Vector3 (*)(void*)) getAbsoluteAddress(targetLibName, 0x1745370);
    
    // //Camera.get_main();
    // get_camera = (void *(*)()) getAbsoluteAddress(targetLibName, 0x172DD64);
    
    // //Camera.WorldToScreenPoint(Vector3 position);
    // get_screenpos = (Vector3 (*)(void *, Vector3)) getAbsoluteAddress(targetLibName, 0x172D88C);
    
    // //Player.Update();
    // MSHookFunction((void *) getAbsoluteAddress(targetLibName, 0x5706A4),
    // (void *) &Player_update,
    // (void **) &old_Player_update);
}
