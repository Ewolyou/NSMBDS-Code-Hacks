#pragma once

#include "nsmb.hpp"

//#define NOTE_BLOCK_FILE_ID 1958

#define BOUNCE_FRAMES 5
#define BOUNCE_FRAMES_HIGH 7
//#define MAX_VELOCITY_Y_NOTE_BLOCK 60fx
#define HIT_COOLDOWN 15
#define HIT_MOVE_PIXEL 3

#define TOP 1
#define BOTTOM 2
//#define LEFT 3
//#define RIGHT 4

class noteBlockActor : public StageEntity {
public:
    s32 onCreate() override;
    static bool loadResources();
    s32 onDestroy() override;
    s32 onRender() override;
    bool updateMain() override;

	u8 hitCooldown;

    static constexpr u16 objectID = 46;

    static constexpr ObjectInfo objectInfo = {};

    static constexpr u16 updatePriority = objectID;
    static constexpr u16 renderPriority = objectID;

    static constexpr ActorProfile profile = {
        &constructObject<noteBlockActor>,
        updatePriority,
        renderPriority,
        loadResources
    };

private:
    static void onBlockBottom(StageActor& self, StageActor& other);
	static void onBlockTop(StageActor& self, StageActor& other);
	//static void onBlockSide(StageActor& self, StageActor& other);

	s32 baseActorX;
	s32 baseActorY;
	u8 hitDirection;

	Collider collider;
};
