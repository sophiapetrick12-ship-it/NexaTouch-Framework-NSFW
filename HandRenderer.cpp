#include "HandRenderer.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <string>
namespace {
RE::NiPointer<RE::NiAVObject> g_handClone;
RE::NiPointer<RE::NiNode> g_handParent;
RE::NiTransform g_handBoneInCloneRoot;
bool g_handAttached{false};
struct HandGeometryFilter {
 std::uint32_t visibleHandGeometry{0};
 void Visit(RE::NiAVObject* object) {
  if (!object) return;
  if (auto* geometry=object->AsGeometry()) {
   const char* raw=geometry->name.c_str(); std::string name=raw?raw:"";
   std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
   const bool keep=name.find("hand")!=std::string::npos || name.find("wrist")!=std::string::npos || name.find("forearm")!=std::string::npos;
   geometry->SetAppCulled(!keep); if(keep) ++visibleHandGeometry;
  }
  if(auto* node=object->AsNode()){auto& children=node->GetChildren();for(std::uint16_t i=0;i<children.size();++i)if(children[i])Visit(children[i].get());}
 }
};
void PlaceHand(float horizontal,float vertical,float stroke) noexcept {
 if(!g_handAttached||!g_handClone||!g_handParent)return;
 horizontal=(std::clamp)(horizontal,-1.0F,1.0F); vertical=(std::clamp)(vertical,-1.0F,1.0F); stroke=(std::clamp)(stroke,0.0F,1.0F);
 RE::NiTransform contact; contact.rotate=RE::NiMatrix3();
 contact.translate=RE::NiPoint3(horizontal*4.0F,vertical*2.5F,5.0F+stroke*1.25F);
 g_handClone->local=contact*g_handBoneInCloneRoot.Invert();
}
}
namespace NexaTouch {
void DetachPlayerHandFromTarget(){
 if(g_handParent&&g_handClone)g_handParent->DetachChild(g_handClone.get());
 g_handClone.reset();g_handParent.reset();g_handAttached=false;g_handBoneInCloneRoot=RE::NiTransform();
}
bool AttachPlayerHandToTarget(RE::Actor& player,RE::Actor& target){
 DetachPlayerHandFromTarget();
 auto* playerRoot=player.Get3D();auto* targetRoot=target.Get3D();if(!playerRoot||!targetRoot)return false;
 auto* playerHand=playerRoot->GetObjectByName(RE::BSFixedString("NPC R Hand [RHnd]"));
 auto* headObject=targetRoot->GetObjectByName(RE::BSFixedString("NPC Head [Head]"));
 auto* head=headObject?headObject->AsNode():nullptr;
 if(!playerHand||!head){spdlog::warn("Pet Head unavailable: XPMSSE right-hand bone or NPC head node missing.");return false;}
 RE::NiPointer<RE::NiAVObject> clone{static_cast<RE::NiAVObject*>(playerRoot->Clone())};
 if(!clone||!clone->AsNode())return false;
 auto* clonedHand=clone->GetObjectByName(RE::BSFixedString("NPC R Hand [RHnd]"));if(!clonedHand)return false;
 HandGeometryFilter filter;filter.Visit(clone.get());
 if(filter.visibleHandGeometry==0){spdlog::warn("Pet Head unavailable: no separately named hand/wrist/forearm geometry in player 3D.");return false;}
 g_handBoneInCloneRoot=clone->world.Invert()*clonedHand->world;
 head->AttachChild(clone.get(),false);g_handClone=clone;g_handParent.reset(head);g_handAttached=true;
 PlaceHand(0.0F,0.0F,0.0F);
 spdlog::info("Pet Head: attached player's own hand appearance ({} visible hand/wrist/forearm geometry objects).",filter.visibleHandGeometry);
 return true;
}
void UpdatePetHeadHand(float horizontal,float vertical,float stroke) noexcept{PlaceHand(horizontal,vertical,stroke);}
}
