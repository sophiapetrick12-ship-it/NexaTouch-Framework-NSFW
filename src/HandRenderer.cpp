#include "HandRenderer.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
namespace {
RE::NiPointer<RE::NiAVObject> g_handClone;
RE::NiPointer<RE::NiNode> g_handParent;
RE::NiTransform g_handBoneInCloneRoot;
RE::ObjectRefHandle g_handTarget;
bool g_handAttached{false};
struct HandGeometryFilter {
 std::uint32_t visibleHandGeometry{0};
 void Visit(RE::NiAVObject* object) {
  if(!object)return;
  if(auto* geometry=object->AsGeometry()){
   const char* raw=geometry->name.c_str();std::string name=raw?raw:"";
   std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
   const bool keep=name.find("hand")!=std::string::npos||name.find("wrist")!=std::string::npos||name.find("forearm")!=std::string::npos;
   geometry->SetAppCulled(!keep);if(keep)++visibleHandGeometry;
  }
  if(auto* node=object->AsNode()){auto& children=node->GetChildren();for(std::uint16_t i=0;i<children.size();++i)if(children[i])Visit(children[i].get());}
 }
};
bool ReadHeadContact(RE::Actor& target,RE::NiPoint3& local) noexcept {
 auto* pick=RE::CrosshairPickData::GetSingleton();auto* root=target.Get3D();if(!pick||!root)return false;
 auto picked=pick->GetActiveTarget().get();if(picked!=static_cast<RE::TESObjectREFR*>(&target))return false;
 auto* object=root->GetObjectByName(RE::BSFixedString("NPC Head [Head]"));auto* head=object?object->AsNode():nullptr;if(!head)return false;
 const auto& p=pick->collisionPoint;
 if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||p.GetDistance(head->world.translate)>24.0F)return false;
 local=head->world.Invert()*p;
 return std::isfinite(local.x)&&std::isfinite(local.y)&&std::isfinite(local.z);
}
void PlaceHand(const RE::NiPoint3& point) noexcept {
 if(!g_handAttached||!g_handClone||!g_handParent)return;
 RE::NiTransform contact;contact.rotate=RE::NiMatrix3();contact.translate=point+RE::NiPoint3(0.0F,0.0F,0.8F);
 g_handClone->local=contact*g_handBoneInCloneRoot.Invert();
}
}
namespace NexaTouch {
bool IsPetHeadContact(RE::Actor& target,RE::NiPoint3& local) noexcept{return ReadHeadContact(target,local);}
void DetachPlayerHandFromTarget(){
 if(g_handParent&&g_handClone)g_handParent->DetachChild(g_handClone.get());
 g_handClone.reset();g_handParent.reset();g_handTarget.reset();g_handAttached=false;g_handBoneInCloneRoot=RE::NiTransform();
}
bool AttachPlayerHandToTarget(RE::Actor& player,RE::Actor& target){
 DetachPlayerHandFromTarget();auto* playerRoot=player.Get3D();auto* targetRoot=target.Get3D();if(!playerRoot||!targetRoot)return false;
 auto* playerHand=playerRoot->GetObjectByName(RE::BSFixedString("NPC R Hand [RHnd]"));auto* headObject=targetRoot->GetObjectByName(RE::BSFixedString("NPC Head [Head]"));auto* head=headObject?headObject->AsNode():nullptr;
 if(!playerHand||!head){spdlog::warn("Pet Head unavailable: required hand/head skeleton node missing.");return false;}
 RE::NiPointer<RE::NiAVObject> clone{static_cast<RE::NiAVObject*>(playerRoot->Clone())};if(!clone||!clone->AsNode())return false;
 auto* clonedHand=clone->GetObjectByName(RE::BSFixedString("NPC R Hand [RHnd]"));if(!clonedHand)return false;
 HandGeometryFilter filter;filter.Visit(clone.get());if(!filter.visibleHandGeometry)return false;
 g_handBoneInCloneRoot=clone->world.Invert()*clonedHand->world;
 head->AttachChild(clone.get(),false);g_handClone=clone;g_handParent.reset(head);g_handTarget=target.GetHandle();g_handAttached=true;
 spdlog::info("Pet Head: attached cloned equipped hand geometry count={}",filter.visibleHandGeometry);return true;
}
void UpdatePetHeadHand(float horizontal,float vertical,float stroke) noexcept {
 if(!g_handAttached||!g_handClone||!g_handParent)return;
 auto reference=g_handTarget.get();auto* actor=reference?reference->As<RE::Actor>():nullptr;if(!actor)return;
 RE::NiPoint3 contact;if(!ReadHeadContact(*actor,contact))return;
 contact.x+=std::clamp(horizontal,-1.0F,1.0F)*4.0F;contact.y+=std::clamp(vertical,-1.0F,1.0F)*2.5F;contact.z+=std::clamp(stroke,0.0F,1.0F)*0.35F;
 PlaceHand(contact);
}
}
