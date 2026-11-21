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
bool g_handAttached{false};\nfloat g_cursorX{0.0F},g_cursorY{0.0F},g_viewWidth{1.0F},g_viewHeight{1.0F};
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
bool ReadHeadContact(RE::Actor& target, float mouseX, float mouseY, float viewportWidth, float viewportHeight, RE::NiPoint3& local) noexcept {
 auto* root=target.Get3D();
 auto* playerCamera=RE::PlayerCamera::GetSingleton();
 if(!root||!playerCamera||!playerCamera->cameraRoot||
    !std::isfinite(mouseX)||!std::isfinite(mouseY)||
    !std::isfinite(viewportWidth)||!std::isfinite(viewportHeight)||
    viewportWidth<2.0F||viewportHeight<2.0F) return false;

 auto* cameraRoot=playerCamera->cameraRoot.get();
 RE::NiCamera* camera=nullptr;
 if(cameraRoot) {
  auto& children=cameraRoot->GetChildren();
  for(std::uint16_t i=0;i<children.size();++i) {
   if(children[i]) {
    camera=RE::skyrim_cast<RE::NiCamera*>(children[i].get());
    if(camera) break;
   }
  }
 }
 if(!camera) return false;

 auto* object=root->GetObjectByName(RE::BSFixedString("NPC Head [Head]"));
 auto* head=object?object->AsNode():nullptr;
 if(!head||!std::isfinite(head->worldBound.radius)||head->worldBound.radius<=1.0F) return false;

 RE::NiPoint3 rayOrigin,rayDirection;
 if(!camera->WindowPointToRay(
      static_cast<std::int32_t>(mouseX),static_cast<std::int32_t>(mouseY),
      rayOrigin,rayDirection,viewportWidth,viewportHeight)) return false;

 const auto toCenter=head->worldBound.center-rayOrigin;
 const float along=toCenter.x*rayDirection.x+toCenter.y*rayDirection.y+toCenter.z*rayDirection.z;
 if(!std::isfinite(along)||along<=0.0F) return false;
 const auto closest=rayOrigin+rayDirection*along;
 const float distance=closest.GetDistance(head->worldBound.center);
 const float radius=head->worldBound.radius;
 if(!std::isfinite(distance)||distance>radius) return false;
 const float chord=std::sqrt((std::max)(0.0F,radius*radius-distance*distance));
 const float hitDistance=along-chord;
 if(hitDistance<=0.0F) return false;
 const auto worldContact=rayOrigin+rayDirection*hitDistance;
 local=head->world.Invert()*worldContact;
 return std::isfinite(local.x)&&std::isfinite(local.y)&&std::isfinite(local.z);
}
void PlaceHand(const RE::NiPoint3& point) noexcept {
 if(!g_handAttached||!g_handClone||!g_handParent)return;
 RE::NiTransform contact;contact.rotate=RE::NiMatrix3();contact.translate=point+RE::NiPoint3(0.0F,0.0F,0.8F);
 g_handClone->local=contact*g_handBoneInCloneRoot.Invert();
}
}
namespace NexaTouch {
bool IsPetHeadContact(RE::Actor& target,float mouseX,float mouseY,float viewportWidth,float viewportHeight,RE::NiPoint3& local) noexcept{return ReadHeadContact(target,mouseX,mouseY,viewportWidth,viewportHeight,local);}
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
void UpdatePetHeadHand(float horizontal,float vertical,float stroke,float mouseX,float mouseY,float viewportWidth,float viewportHeight) noexcept {
 if(!g_handAttached||!g_handClone||!g_handParent)return;
 auto reference=g_handTarget.get();auto* actor=reference?reference->As<RE::Actor>():nullptr;if(!actor)return;
 RE::NiPoint3 contact;if(!ReadHeadContact(*actor,mouseX,mouseY,viewportWidth,viewportHeight,contact))return;
 contact.x+=std::clamp(horizontal,-1.0F,1.0F)*4.0F;contact.y+=std::clamp(vertical,-1.0F,1.0F)*2.5F;contact.z+=std::clamp(stroke,0.0F,1.0F)*0.35F;
 PlaceHand(contact);
}
}
