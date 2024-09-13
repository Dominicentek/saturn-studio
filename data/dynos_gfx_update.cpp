#include "behavior_data.h"
#include "saturn/saturn_actors.h"
#include "dynos.cpp.h"
extern "C" {
#include "object_fields.h"
#include "game/level_update.h"
#include "game/object_list_processor.h"
}

//
// Update animations
//

// Retrieve the current Mario's animation index
static s32 RetrieveCurrentMarioAnimationIndex() {
    struct MarioAnimDmaRelatedThing *_AnimDmaTable = gMarioState->animation->animDmaTable;
    for (s32 i = 0; i != (s32) _AnimDmaTable->count; ++i) {
        void *_AnimAddr = _AnimDmaTable->srcAddr + _AnimDmaTable->anim[i].offset;
        if (_AnimAddr == gMarioState->animation->currentAnimAddr) {
            return i;
        }
    }
    return -1;
}

// Retrieve the current animation index
// As we don't know the length of the table, let's hope that we'll always find the animation...
static s32 RetrieveCurrentAnimationIndex(struct Object *aObject) {
    if (!aObject->oAnimations || !aObject->header.gfx.unk38.curAnim) {
        return -1;
    }
    for (s32 i = 0; aObject->oAnimations[i] != NULL; ++i) {
        if (aObject->oAnimations[i] == aObject->header.gfx.unk38.curAnim) {
            return i;
        }
    }
    return -1;
}

// Must be called twice, before and after geo_set_animation_globals
void DynOS_Gfx_SwapAnimations(void *aPtr) {
    static Animation *pDefaultAnimation = NULL;
    static Animation  sGfxDataAnimation;

    // Does the object has a model?
    struct Object *_Object = (struct Object *) aPtr;
    if (!_Object->header.gfx.sharedChild) {
        return;
    }

    const Array<PackData *> &pDynosPacks = DynOS_Gfx_GetPacks();

    // Swap the current animation with the one from the Gfx data
    if (!pDefaultAnimation) {
        pDefaultAnimation = _Object->header.gfx.unk38.curAnim;

        // Mario actor
        if (_Object->behavior != bhvMarioActor) return;
        MarioActor* _Actor = saturn_get_actor(_Object->oMarioActorIndex);
        if (!_Actor) {
            return;
        }

        if (_Actor->selected_model == -1 || _Actor->custom_bone || _Actor->animstate.custom) {
            return;
        }

        // Gfx data
        GfxData *_GfxData = DynOS_Gfx_LoadFromBinary(pDynosPacks[_Actor->selected_model]->mPath, "mario_geo");
        if (!_GfxData) {
            return;
        }

        // Animation table
        if (_GfxData->mAnimationTable.Empty()) {
            return;
        }

        // Animation index
        s32 _AnimIndex = _Actor->animstate.id;

        // Animation data
        const AnimData *_AnimData = (const AnimData *) _GfxData->mAnimationTable[_AnimIndex].second;
        if (_AnimData) {
            sGfxDataAnimation.flags = 4;
            sGfxDataAnimation.unk02 = _AnimData->mUnk02;
            sGfxDataAnimation.unk04 = _AnimData->mUnk04;
            sGfxDataAnimation.unk06 = _AnimData->mUnk06;
            sGfxDataAnimation.unk08 = _AnimData->mUnk08;
            sGfxDataAnimation.unk0A = _AnimData->mUnk0A.second;
            sGfxDataAnimation.values = _AnimData->mValues.second.begin();
            sGfxDataAnimation.index = _AnimData->mIndex.second.begin();
            sGfxDataAnimation.length = _AnimData->mLength;
            _Object->header.gfx.unk38.curAnim = &sGfxDataAnimation;
            _Actor->animstate.length = _AnimData->mUnk0A.second;
        }

    // Restore the default animation
    } else {
        _Object->header.gfx.unk38.curAnim = pDefaultAnimation;
        pDefaultAnimation = NULL;
    }
}

//
// Update models
//

void DynOS_Gfx_Update() {
    if (gMarioObject) {

        // Loop through all object lists
        MarioActor* _Actor = gMarioActorList;
        while (_Actor) {
            if (!_Actor->exists) {
                _Actor = _Actor->next;
                continue;
            }

            struct Object* _Object = _Actor->marioObj;
            if (_Actor->obj_model != MODEL_MARIO) {
                _Actor = _Actor->next;
                continue;
            }

            const Array<PackData *> &pDynosPacks = DynOS_Gfx_GetPacks();

            if (_Object->header.gfx.sharedChild) {
                if (_Object->header.gfx.sharedChild && _Actor->selected_model != -1) {
                    GfxData* _GfxData = DynOS_Gfx_LoadFromBinary(pDynosPacks[_Actor->selected_model]->mPath, "mario_geo");
                    _Object->header.gfx.sharedChild = (GraphNode*)DynOS_Geo_GetGraphNode((*(_GfxData->mGeoLayouts.end() - 1))->mData, true);
                }
                else {
                    _Object->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_MARIO];
                }
            }

            _Actor = _Actor->next;
        }
    }
}
