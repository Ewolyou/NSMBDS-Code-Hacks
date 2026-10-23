#include "nsmb.hpp"
#include <nsmb/core/graphics/2d/oam.hpp>
#include "noteBlockActor.hpp"

extern "C" {
#include "note_block.h"
}

#define TILE_INDEX	0x200
#define PAL_SLOT	15

#define OAM_MODE_NORMAL		0
#define OAM_SHAPE_SQUARE	0
#define OAM_SIZE_16x16		(1u << 30)
#define OAM_COLOR_16		0


ncp_over(0x020c560c, 0)
const ObjectInfo objectInfo = noteBlockActor::objectInfo; //Stage Object ID 44 (use this in the editor)

ncp_over(0x02039a34)
static constexpr const ActorProfile* profile = &noteBlockActor::profile; //objectID 46

static constexpr s32 noteBlockSpringSFX = SeqArc::SAR_VS_COMMON_PLAYER_BASE::SE_PLY_JUMPDAI;
static constexpr s32 noteBlockSpringHighSFX = SeqArc::SAR_VS_COMMON_PLAYER_BASE::SE_OBJ_TRAMPOLINE_JUMP2;

static u8 bounce[2] = {0, 0};
static u8 bounceFrames[2] = {0, 0};
static noteBlockActor* bounceSource[2] = {nullptr, nullptr};

s32 noteBlockActor::onCreate() {

	collider.initEx(
		this,
		0 * FX32_ONE,  0 * FX32_ONE,
		16 * FX32_ONE, -16 * FX32_ONE,
		&noteBlockActor::onBlockTop,
		&noteBlockActor::onBlockBottom,
		nullptr,     //&noteBlockActor::onBlockSide,
		TileType::Solid,
		0
	);

    collider.link();

	baseActorX = this->position.x;
	baseActorY = this->position.y;
	hitCooldown = 0;
	hitDirection = 0;

	return true;
}

bool noteBlockActor::loadResources() {
    return true;
}

s32 noteBlockActor::onDestroy() {
	for(int i = 0; i < 2; i++)
		if(bounceSource[i] == this) bounceSource[i] = nullptr;
    collider.unlink();
    return true;
}

/* onRender by Claude */
s32 noteBlockActor::onRender() {
	u32 unit = 32u << ((REG_DISPCNT >> 20) & 3);

	volatile u32* dst = (volatile u32*)((u32)HW_OBJ_VRAM_ADDR + TILE_INDEX * unit);
	for (u32 i = 0; i < note_blockTilesLen / 4; i++)
		dst[i] = note_blockTiles[i];

	GX_LoadOBJPltt((void*)note_blockPal, PAL_SLOT * 32, 32);

	static GXOamAttr note_block[1];
	note_block[0] = OAM::getOBJAttr(
		0, 0, 0,
		OAM_MODE_NORMAL,
		false,
		GX_OAM_EFFECT_NONE,
		OAM_SHAPE_SQUARE | OAM_SIZE_16x16,
		OAM_COLOR_16,
		TILE_INDEX,
		PAL_SLOT,
		0,
		0x8000
	);

	Vec2 scale;
	scale.x = FX32_ONE;
	scale.y = FX32_ONE;
	s16 rotCenter[2] = { 0, 0 };

	OAM::drawSprite(
		note_block,
		this->position.x, this->position.y,
		OAM::Flags::Prio1,
		0, 0,
		&scale, 0, rotCenter,
		OAM::Settings::None
	);

	return true;
}

bool noteBlockActor::updateMain() {
	s32 movePixels = HIT_MOVE_PIXEL;// / (HIT_COOLDOWN/2);

	if(hitCooldown > 0) {
		if(hitCooldown > HIT_COOLDOWN/2) {
			if(hitDirection == BOTTOM) this->position.y = baseActorY + (movePixels * FX32_ONE);
			else if(hitDirection == TOP) this->position.y = baseActorY - (movePixels * FX32_ONE);
			//else if(hitDirection == LEFT) this->position.x = baseActorX - (movePixels * FX32_ONE);
			//else if(hitDirection == RIGHT) this->position.x = baseActorX + (movePixels * FX32_ONE);
		}
		else {
			if(hitDirection == BOTTOM) {
				this->position.y = baseActorY - (movePixels * FX32_ONE);
			}
			else if(hitDirection == TOP) {
				this->position.y = baseActorY + (movePixels * FX32_ONE);
			}
			//else if(hitDirection == LEFT) this->position.x = baseActorX + (movePixels * FX32_ONE);
			//else if(hitDirection == RIGHT) this->position.x = baseActorX - (movePixels * FX32_ONE);
		}
	}
	else {
		this->position.x = baseActorX;
		this->position.y = baseActorY;
	}

	if(hitCooldown > 0) hitCooldown--;

	collider.updatePosition();

    return true;
}

void noteBlockActor::onBlockBottom(StageActor& self, StageActor& other) {
	auto& actor = static_cast<noteBlockActor&>(self);
	if(actor.hitCooldown <= 0) {
		actor.hitCooldown = HIT_COOLDOWN;
		actor.hitDirection = BOTTOM;
	}
}

void noteBlockActor::onBlockTop(StageActor& self, StageActor& other) {
	auto& actor = static_cast<noteBlockActor&>(self);
	
	if(actor.hitCooldown <= 0) {
		//if(player->subActionFlag.groundPounding) {
			actor.hitCooldown = HIT_COOLDOWN;
			actor.hitDirection = TOP;
		//}
	}

	if(other.actorType != ActorType::Player) return; // only player can bounce
	auto& player = static_cast<Player&>(other);

	if(actor.hitCooldown == (HIT_COOLDOWN/2)) {
		if((Input::playerKeysHeld[player.playerID] & (Keys::A | Keys::B)) ||
		   (Input::playerKeysPressed[player.playerID] & (Keys::A | Keys::B))) bounce[player.playerID] = 2;
		else bounce[player.playerID] = 1;
	}
}

ncp_hook(0x020fd1d4,10) // Player::onUpdate
static void doPlayerRelatedStuff(Player *player) {
	
	if(bounce[player->playerID] == 1) {
		
		if(!player->physicsFlag.lowHitbox && !player->subActionFlag.groundPounding) {
			Sound::playSFX(noteBlockSpringSFX, &player->position);
			player->setAnimation(5, true, Player::FrameMode::Continue, 1fx, 0);
			player->velocity.y = 0;
			player->position.y += 5fx;
		}
		//else player->velocity.y = 0.25fx;

		bounce[player->playerID] = 0;
		bounceFrames[player->playerID] = BOUNCE_FRAMES;
	}
	else if(bounce[player->playerID] == 2) {
		
		if(!player->physicsFlag.lowHitbox && !player->subActionFlag.groundPounding) {
			Sound::playSFX(noteBlockSpringHighSFX, &player->position);
			player->setAnimation(5, true, Player::FrameMode::Continue, 1fx, 0);
			player->velocity.y = 0;
			player->position.y += 5fx;
		}
		//else player->velocity.y = 1fx;

		bounce[player->playerID] = -1;
		bounceFrames[player->playerID] = BOUNCE_FRAMES_HIGH;
	}

	if(bounceFrames[player->playerID] > 0 && !player->physicsFlag.lowHitbox) {
		//if(bounceFrames[player->playerID] > BOUNCE_FRAMES) player->velocity.y += 2fx;
		//else

		noteBlockActor* src = bounceSource[player->playerID];

		if(!( (Input::playerKeysHeld[player->playerID] & (Keys::A | Keys::B)) &&
			   src && src->hitCooldown < HIT_COOLDOWN/2) ) {
			if(!player->physicsFlag.lowHitbox && !player->subActionFlag.groundPounding && !player->physicsFlag.groundPoundPreLand) {
				if(bounce[player->playerID] == -1) player->velocity.y += 2fx;
				else player->velocity.y += 1fx;
			}
		}
	}

	if(bounceFrames[player->playerID] > 0) bounceFrames[player->playerID]--;
	else bounce[player->playerID] = 0;

	/*if(player->velocity.y > MAX_VELOCITY_Y_NOTE_BLOCK)
		player->velocity.y = MAX_VELOCITY_Y_NOTE_BLOCK;*/
}

/*
void noteBlockActor::onBlockSide(StageActor& self, StageActor& other) {
	auto& actor = static_cast<noteBlockActor&>(self);
    if(actor.hitCooldown <= 0) {
		const int hitFromLeft = (static_cast<u8>(actor.collider.updateFlag) & static_cast<u8>(Collider::UpdateFlag::DirectionLeft)) != 0;

		if(hitFromLeft) {
			actor.hitCooldown = HIT_COOLDOWN;
			actor.hitDirection = LEFT;
		}
		else {
			actor.hitCooldown = HIT_COOLDOWN;
			actor.hitDirection = RIGHT;
		}

		actor.collider.updatePosition();
	}
}
*/
