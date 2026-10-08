#include <atomic>
#include <string>
#include <vector>

#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/System.hh>
#include <gz/sim/Util.hh>
#include <gz/sim/components/AngularVelocity.hh>
#include <gz/sim/components/Collision.hh>
#include <gz/sim/components/DetachableJoint.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/LinearVelocity.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/ParentEntity.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/msgs/empty.pb.h>
#include <gz/plugin/Register.hh>
#include <gz/transport/Node.hh>

namespace pnp_hold {

class PnpHold
  : public gz::sim::System,
    public gz::sim::ISystemConfigure,
    public gz::sim::ISystemPreUpdate {
 public:
  void Configure(const gz::sim::Entity & entity,
                 const std::shared_ptr<const sdf::Element> & sdf,
                 gz::sim::EntityComponentManager & ecm,
                 gz::sim::EventManager &) override {
    model_ = gz::sim::Model(entity);
    parent_name_ = sdf->Get<std::string>("parent_link");
    child_model_ = sdf->Get<std::string>("child_model");
    child_link_ = sdf->Get<std::string>("child_link");
    parent_ = model_.LinkByName(ecm, parent_name_);
    node_.Subscribe(sdf->Get<std::string>("attach_topic"), &PnpHold::OnAttach, this);
    node_.Subscribe(sdf->Get<std::string>("detach_topic"), &PnpHold::OnDetach, this);
    gzmsg << "pnp hold ready " << child_model_ << std::endl;
  }

  void PreUpdate(const gz::sim::UpdateInfo &,
                 gz::sim::EntityComponentManager & ecm) override {
    if (attach_.exchange(false) && joint_ == gz::sim::kNullEntity) {
      ResolveChild(ecm);
      if (parent_ == gz::sim::kNullEntity || child_ == gz::sim::kNullEntity) {
        gzwarn << "pnp hold missed links for " << child_model_ << std::endl;
        return;
      }
      ParkCollisions(ecm, true);
      joint_ = ecm.CreateEntity();
      gz::sim::components::DetachableJointInfo info;
      info.parentLink = parent_;
      info.childLink = child_;
      info.jointType = "fixed";
      ecm.CreateComponent(joint_, gz::sim::components::DetachableJoint(info));
      gzmsg << "pnp hold attached " << child_model_ << std::endl;
    }
    if (detach_.exchange(false) && joint_ != gz::sim::kNullEntity) {
      ecm.RequestRemoveEntity(joint_);
      joint_ = gz::sim::kNullEntity;
      ParkCollisions(ecm, false);
      ZeroVelocity(ecm, child_);
      gzmsg << "pnp hold detached " << child_model_ << std::endl;
    }
  }

 private:
  void OnAttach(const gz::msgs::Empty &) { attach_ = true; }
  void OnDetach(const gz::msgs::Empty &) { detach_ = true; }

  void ResolveChild(gz::sim::EntityComponentManager & ecm) {
    child_ = gz::sim::kNullEntity;
    for (const auto entity : gz::sim::entitiesFromScopedName(child_model_, ecm)) {
      if (!ecm.EntityHasComponentType(entity, gz::sim::components::Model::typeId)) {
        continue;
      }
      const auto link = ecm.EntityByComponents(
        gz::sim::components::Link(),
        gz::sim::components::ParentEntity(entity),
        gz::sim::components::Name(child_link_));
      if (link != gz::sim::kNullEntity) {
        child_ = link;
        return;
      }
    }
  }

  void ParkCollisions(gz::sim::EntityComponentManager & ecm, bool park) {
    if (park) {
      parked_.clear();
      ecm.Each<gz::sim::components::Collision,
               gz::sim::components::ParentEntity,
               gz::sim::components::Pose>(
        [&](const gz::sim::Entity & entity,
            const gz::sim::components::Collision *,
            const gz::sim::components::ParentEntity * parent,
            const gz::sim::components::Pose * pose) {
          if (parent->Data() != child_) {
            return true;
          }
          parked_.push_back({entity, pose->Data()});
          ecm.Component<gz::sim::components::Pose>(entity)->Data() =
            gz::math::Pose3d(0, 0, -100, 0, 0, 0);
          ecm.SetChanged(entity, gz::sim::components::Pose::typeId,
                         gz::sim::ComponentState::OneTimeChange);
          return true;
        });
      return;
    }
    for (const auto & saved : parked_) {
      auto * pose = ecm.Component<gz::sim::components::Pose>(saved.entity);
      if (pose != nullptr) {
        pose->Data() = saved.pose;
        ecm.SetChanged(saved.entity, gz::sim::components::Pose::typeId,
                       gz::sim::ComponentState::OneTimeChange);
      }
    }
    parked_.clear();
  }

  void ZeroVelocity(gz::sim::EntityComponentManager & ecm,
                    gz::sim::Entity link) {
    auto * linear = ecm.Component<gz::sim::components::LinearVelocity>(link);
    if (linear != nullptr) {
      linear->Data() = gz::math::Vector3d::Zero;
    }
    auto * angular = ecm.Component<gz::sim::components::AngularVelocity>(link);
    if (angular != nullptr) {
      angular->Data() = gz::math::Vector3d::Zero;
    }
  }

  gz::sim::Model model_{gz::sim::kNullEntity};
  gz::transport::Node node_;
  std::string parent_name_;
  std::string child_model_;
  std::string child_link_;
  gz::sim::Entity parent_{gz::sim::kNullEntity};
  gz::sim::Entity child_{gz::sim::kNullEntity};
  gz::sim::Entity joint_{gz::sim::kNullEntity};
  std::atomic<bool> attach_{false};
  std::atomic<bool> detach_{false};
  struct SavedPose {
    gz::sim::Entity entity;
    gz::math::Pose3d pose;
  };
  std::vector<SavedPose> parked_;
};

}

GZ_ADD_PLUGIN(
  pnp_hold::PnpHold,
  gz::sim::System,
  pnp_hold::PnpHold::ISystemConfigure,
  pnp_hold::PnpHold::ISystemPreUpdate)
