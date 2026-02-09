#include "Canvas/ESP.h"
#include "Canvas/Bools.h"
#include "Canvas/StructsCommon.h"

#include "DarkManager.h"

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
  
         esp.DrawText(Color::Red(), Allplayers.c_str(), Vector2(screenWidth / 2 ,screenHeight / 2), 35.0f);      
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
    

      //UnityEngine class Component public Transform get_transform() { };
     get_Transform = (void *(*)(void *)) getAbsoluteAddress(targetLibName, 0x239C588);
    
     //UnityEngine class Transform public Vector3 get_position() { };
     get_position = (Vector3 (*)(void*)) getAbsoluteAddress(targetLibName, 0x23AEEC4);
    
     //UnityEngine public sealed class Camera public static Camera get_main() { };
     get_camera = (void *(*)()) getAbsoluteAddress(targetLibName, 0x2364B78);
    
     //UnityEngine public sealed class Camera public Vector3 WorldToScreenPoint(Vector3 position) { };
     get_screenpos = (Vector3 (*)(void *, Vector3)) getAbsoluteAddress(targetLibName, 0x2364804);
    
    // public class EnemyAIGranny : MonoBehaviour public virtual void FixedUpdate() { };
    MSHookFunction((void *) getAbsoluteAddress(targetLibName, 0xA257CC),
        (void *) &Player_update,
        (void **) &old_Player_update);
}