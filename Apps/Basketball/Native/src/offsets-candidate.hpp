#pragma once
/*
/* =============================================================
/*  rbxoffsets — composed offsets
/*  
/*  Roblox Version  : version-4ea8e85481e34f46
/*  Total Offsets   : 1740
/*  Composed At     : 2026-11-10T19:11:33.982Z
/*  
/*  Composed from a verified dump by rbxoffsets and a mirrored
/*  dump from another source. Where both name an offset and disagree,
/*  the mirrored value is served; ours fills only its gaps and zeros.
/*  
/*  verified only 1345   mirrored only 245   agreed 129
/*  disputed 8   repaired 1   unknown 12
/* =============================================================
*/

#include <cstdint>
#include <string>
namespace Offsets {
    inline std::string ClientVersion = "version-4ea8e85481e34f46";

    namespace AccessoryDescription {
         inline constexpr uintptr_t AssetId = 0xa8;
         inline constexpr uintptr_t IsLayered = 0xe0;
         inline constexpr uintptr_t Puffiness = 0xdc;
         inline constexpr uintptr_t Scale = 0xc8;
    }

    namespace Accoutrement {
         inline constexpr uintptr_t AttachmentPoint = 0xa0;
         inline constexpr uintptr_t BackendAccoutrementState = 0xd0;
    }

    namespace AdPlacement {
         inline constexpr uintptr_t PlacementId = 0x118;
         inline constexpr uintptr_t Visible = 0x124;
    }

    namespace AdPortal {
         inline constexpr uintptr_t PortalVersion = 0xc0;
    }

    namespace AirController {
         inline constexpr uintptr_t BalanceMaxTorque = 0xd4;
         inline constexpr uintptr_t BalanceSpeed = 0xd8;
         inline constexpr uintptr_t MaintainAngularMomentum = 0xe8;
         inline constexpr uintptr_t MaintainLinearMomentum = 0xe9;
         inline constexpr uintptr_t TurnMaxTorque = 0xe0;
         inline constexpr uintptr_t TurnSpeedFactor = 0xe4;
    }

    namespace AirProperties {
         inline constexpr uintptr_t AirDensity = 0x18;
         inline constexpr uintptr_t GlobalWind = 0x3c;
    }

    namespace AlignOrientation {
         inline constexpr uintptr_t CFrame = 0x158;
         inline constexpr uintptr_t MaxAngularVelocity = 0x18c;
         inline constexpr uintptr_t MaxTorque = 0x190;
         inline constexpr uintptr_t PrimaryAxisOnly = 0x19c;
         inline constexpr uintptr_t ReactionTorqueEnabled = 0x19d;
         inline constexpr uintptr_t Responsiveness = 0x198;
         inline constexpr uintptr_t RigidityEnabled = 0x19e;
    }

    namespace AlignPosition {
         inline constexpr uintptr_t MaxForce = 0x178;
         inline constexpr uintptr_t MaxVelocity = 0x17c;
         inline constexpr uintptr_t Responsiveness = 0x184;
         inline constexpr uintptr_t RigidityEnabled = 0x18a;
    }

    namespace AngularVelocity {
         inline constexpr uintptr_t MaxTorque = 0x164;
         inline constexpr uintptr_t ReactionTorqueEnabled = 0x16c;
    }

    namespace AnimationClip {
         inline constexpr uintptr_t Guid = 0xa8;
         inline constexpr uintptr_t Loop = 0xb4;
    }

    namespace AnimationConstraint {
         inline constexpr uintptr_t AngularStrength = 0x15c;
         inline constexpr uintptr_t LinearDamping = 0x160;
         inline constexpr uintptr_t LinearStrength = 0x164;
         inline constexpr uintptr_t MaxForce = 0x168;
    }

    namespace AnimationPlayer {
         inline constexpr uintptr_t AutoPlay = 0x140;
         inline constexpr uintptr_t CurrentFrame = 0x138;
         inline constexpr uintptr_t Enabled = 0x141;
         inline constexpr uintptr_t IsPlaying = 0x142;
         inline constexpr uintptr_t Loop = 0x143;
         inline constexpr uintptr_t Speed = 0x13c;
         inline constexpr uintptr_t TimePosition = 0x130;
    }

    namespace AnimationStreamTrack {
         inline constexpr uintptr_t WeightTarget = 0xa8;
    }

    namespace AnimationTrack {
         inline constexpr uintptr_t Animation = 0xa0;
         inline constexpr uintptr_t Animator = 0xf8;
         inline constexpr uintptr_t IsPlaying = 0xa10;
         inline constexpr uintptr_t Looped = 0xcd;
         inline constexpr uintptr_t Speed = 0xbc;
         inline constexpr uintptr_t TimePosition = 0xc0;
         inline constexpr uintptr_t WeightTarget = 0xc8;
    }

    namespace Animator {
         inline constexpr uintptr_t ActiveAnimations = 0x920;
         inline constexpr uintptr_t AnimTrackMetadata0 = 0x38c;
         inline constexpr uintptr_t AnimTrackMetadata1 = 0x390;
         inline constexpr uintptr_t AnimTrackMetadata10 = 0x394;
         inline constexpr uintptr_t AnimTrackMetadata11 = 0x398;
         inline constexpr uintptr_t AnimTrackMetadata12 = 0x39c;
         inline constexpr uintptr_t AnimTrackMetadata13 = 0x3a0;
         inline constexpr uintptr_t AnimTrackMetadata14 = 0x3a4;
         inline constexpr uintptr_t AnimTrackMetadata15 = 0x3a8;
         inline constexpr uintptr_t AnimTrackMetadata2 = 0x3ac;
         inline constexpr uintptr_t AnimTrackMetadata3 = 0x3b0;
         inline constexpr uintptr_t AnimTrackMetadata4 = 0x3b4;
         inline constexpr uintptr_t AnimTrackMetadata5 = 0x3b8;
         inline constexpr uintptr_t AnimTrackMetadata6 = 0x3bc;
         inline constexpr uintptr_t AnimTrackMetadata7 = 0x3c0;
         inline constexpr uintptr_t AnimTrackMetadata8 = 0x3c4;
         inline constexpr uintptr_t AnimTrackMetadata9 = 0x3c8;
         inline constexpr uintptr_t AnimTrackPlayState0 = 0xd8;
         inline constexpr uintptr_t AnimTrackPlayState1 = 0xe8;
         inline constexpr uintptr_t AnimTrackPlayState10 = 0xf8;
         inline constexpr uintptr_t AnimTrackPlayState11 = 0x108;
         inline constexpr uintptr_t AnimTrackPlayState12 = 0x118;
         inline constexpr uintptr_t AnimTrackPlayState13 = 0x128;
         inline constexpr uintptr_t AnimTrackPlayState14 = 0x138;
         inline constexpr uintptr_t AnimTrackPlayState15 = 0x148;
         inline constexpr uintptr_t AnimTrackPlayState2 = 0x158;
         inline constexpr uintptr_t AnimTrackPlayState3 = 0x168;
         inline constexpr uintptr_t AnimTrackPlayState4 = 0x178;
         inline constexpr uintptr_t AnimTrackPlayState5 = 0x188;
         inline constexpr uintptr_t AnimTrackPlayState6 = 0x198;
         inline constexpr uintptr_t AnimTrackPlayState7 = 0x1a8;
         inline constexpr uintptr_t AnimTrackPlayState8 = 0x1b8;
         inline constexpr uintptr_t AnimTrackPlayState9 = 0x1c8;
         inline constexpr uintptr_t AnimTrackWeight0 = 0x1d8;
         inline constexpr uintptr_t AnimTrackWeight1 = 0x1e8;
         inline constexpr uintptr_t AnimTrackWeight10 = 0x1f8;
         inline constexpr uintptr_t AnimTrackWeight11 = 0x208;
         inline constexpr uintptr_t AnimTrackWeight12 = 0x218;
         inline constexpr uintptr_t AnimTrackWeight13 = 0x228;
         inline constexpr uintptr_t AnimTrackWeight14 = 0x238;
         inline constexpr uintptr_t AnimTrackWeight15 = 0x248;
         inline constexpr uintptr_t AnimTrackWeight2 = 0x258;
         inline constexpr uintptr_t AnimTrackWeight3 = 0x268;
         inline constexpr uintptr_t AnimTrackWeight4 = 0x278;
         inline constexpr uintptr_t AnimTrackWeight5 = 0x288;
         inline constexpr uintptr_t AnimTrackWeight6 = 0x298;
         inline constexpr uintptr_t AnimTrackWeight7 = 0x2a8;
         inline constexpr uintptr_t AnimTrackWeight8 = 0x2b8;
         inline constexpr uintptr_t AnimTrackWeight9 = 0x2c8;
         inline constexpr uintptr_t AnimationId0 = 0x2d8;
         inline constexpr uintptr_t AnimationId1 = 0x2e0;
         inline constexpr uintptr_t AnimationId10 = 0x2e8;
         inline constexpr uintptr_t AnimationId11 = 0x2f0;
         inline constexpr uintptr_t AnimationId12 = 0x2f8;
         inline constexpr uintptr_t AnimationId13 = 0x300;
         inline constexpr uintptr_t AnimationId14 = 0x308;
         inline constexpr uintptr_t AnimationId15 = 0x310;
         inline constexpr uintptr_t AnimationId2 = 0x318;
         inline constexpr uintptr_t AnimationId3 = 0x320;
         inline constexpr uintptr_t AnimationId4 = 0x328;
         inline constexpr uintptr_t AnimationId5 = 0x330;
         inline constexpr uintptr_t AnimationId6 = 0x338;
         inline constexpr uintptr_t AnimationId7 = 0x340;
         inline constexpr uintptr_t AnimationId8 = 0x348;
         inline constexpr uintptr_t AnimationId9 = 0x350;
         inline constexpr uintptr_t PreferLodEnabled = 0x3cc;
         inline constexpr uintptr_t RootMotion = 0x358;
         inline constexpr uintptr_t RootMotionWeight = 0x388;
    }

    namespace ArcHandles {
         inline constexpr uintptr_t MouseButton1DownConnectionCount = 0x10c;
         inline constexpr uintptr_t MouseButton1UpConnectionCount = 0x110;
         inline constexpr uintptr_t MouseDragConnectionCount = 0x114;
         inline constexpr uintptr_t MouseEnterConnectionCount = 0x118;
         inline constexpr uintptr_t MouseLeaveConnectionCount = 0x11c;
    }

    namespace AssetDeliveryProxy {
         inline constexpr uintptr_t Interface = 0xb0;
    }

    namespace AssetService {
         inline constexpr uintptr_t AllowInsertFreeAssets = 0xa8;
    }

    namespace Atmosphere {
         inline constexpr uintptr_t Color = 0xa0;
         inline constexpr uintptr_t Decay = 0xac;
         inline constexpr uintptr_t Density = 0xb8;
         inline constexpr uintptr_t Glare = 0xbc;
         inline constexpr uintptr_t Haze = 0xc0;
         inline constexpr uintptr_t Offset = 0xc4;
    }

    namespace Attachment {
         inline constexpr uintptr_t Position = 0xac;
         inline constexpr uintptr_t SecondaryAxis = 0xb8;
    }

    namespace AudioAnalyzer {
         inline constexpr uintptr_t SpectrumEnabled = 0x124;
    }

    namespace AudioChorus {
         inline constexpr uintptr_t Mix = 0x124;
         inline constexpr uintptr_t Rate = 0x128;
    }

    namespace AudioCompressor {
         inline constexpr uintptr_t Bypass = 0x134;
         inline constexpr uintptr_t MakeupGain = 0x124;
         inline constexpr uintptr_t Ratio = 0x128;
         inline constexpr uintptr_t Release = 0x12c;
         inline constexpr uintptr_t Threshold = 0x130;
    }

    namespace AudioDeviceInput {
         inline constexpr uintptr_t Active = 0x160;
         inline constexpr uintptr_t Muted = 0x164;
         inline constexpr uintptr_t MutedByLocalUser = 0x165;
         inline constexpr uintptr_t NoiseSuppression = 0x166;
         inline constexpr uintptr_t Volume = 0x15c;
    }

    namespace AudioDistortion {
         inline constexpr uintptr_t Bypass = 0x124;
    }

    namespace AudioEcho {
         inline constexpr uintptr_t Bypass = 0x134;
         inline constexpr uintptr_t DryLevel = 0x124;
         inline constexpr uintptr_t Feedback = 0x128;
         inline constexpr uintptr_t RampTime = 0x12c;
         inline constexpr uintptr_t WetLevel = 0x130;
    }

    namespace AudioEmitter {
         inline constexpr uintptr_t AcousticSimulationEnabled = 0x168;
         inline constexpr uintptr_t AudioInteractionGroup = 0x138;
    }

    namespace AudioEqualizer {
         inline constexpr uintptr_t Bypass = 0x134;
         inline constexpr uintptr_t HighGain = 0x128;
         inline constexpr uintptr_t LowGain = 0x12c;
         inline constexpr uintptr_t MidGain = 0x130;
         inline constexpr uintptr_t MidRange = 0x120;
    }

    namespace AudioFader {
         inline constexpr uintptr_t Bypass = 0x124;
    }

    namespace AudioFilter {
         inline constexpr uintptr_t Bypass = 0x130;
         inline constexpr uintptr_t Frequency = 0x124;
         inline constexpr uintptr_t Gain = 0x128;
         inline constexpr uintptr_t Q = 0x12c;
    }

    namespace AudioFlanger {
         inline constexpr uintptr_t Mix = 0x124;
         inline constexpr uintptr_t Rate = 0x128;
    }

    namespace AudioGate {
         inline constexpr uintptr_t Attack = 0x128;
         inline constexpr uintptr_t Bypass = 0x130;
         inline constexpr uintptr_t Release = 0x12c;
         inline constexpr uintptr_t Threshold = 0x120;
    }

    namespace AudioLimiter {
         inline constexpr uintptr_t Bypass = 0x128;
         inline constexpr uintptr_t Release = 0x124;
    }

    namespace AudioListener {
         inline constexpr uintptr_t AudioInteractionGroup = 0x138;
    }

    namespace AudioPitchShifter {
         inline constexpr uintptr_t Bypass = 0x128;
    }

    namespace AudioPlayer {
         inline constexpr uintptr_t Asset = 0x128;
         inline constexpr uintptr_t IsPlaying = 0x15a;
         inline constexpr uintptr_t LoopRegion = 0x140;
         inline constexpr uintptr_t Volume = 0x154;
    }

    namespace AudioRecorder {
         inline constexpr uintptr_t IsRecording = 0x120;
    }

    namespace AudioReverb {
         inline constexpr uintptr_t DecayTime = 0x124;
         inline constexpr uintptr_t Density = 0x128;
         inline constexpr uintptr_t Diffusion = 0x12c;
         inline constexpr uintptr_t DryLevel = 0x130;
         inline constexpr uintptr_t HighCutFrequency = 0x138;
         inline constexpr uintptr_t LateDelayTime = 0x13c;
         inline constexpr uintptr_t LowShelfFrequency = 0x140;
         inline constexpr uintptr_t LowShelfGain = 0x144;
         inline constexpr uintptr_t ReferenceFrequency = 0x148;
         inline constexpr uintptr_t WetLevel = 0x14c;
    }

    namespace AudioSearchParams {
         inline constexpr uintptr_t MaxPageSize = 0x148;
    }

    namespace AudioSpeechToText {
         inline constexpr uintptr_t DictationEnabled = 0x180;
         inline constexpr uintptr_t DisableVoiceDetection = 0x181;
    }

    namespace AudioStreamReader {
         inline constexpr uintptr_t SampleRate = 0x120;
    }

    namespace AudioStreamWriter {
         inline constexpr uintptr_t SampleRate = 0x120;
    }

    namespace AudioTextToSpeech {
         inline constexpr uintptr_t AutoLocalize = 0x178;
         inline constexpr uintptr_t IsPlaying = 0x179;
         inline constexpr uintptr_t Pitch = 0x168;
         inline constexpr uintptr_t PlaybackSpeed = 0x16c;
         inline constexpr uintptr_t Speed = 0x170;
         inline constexpr uintptr_t Volume = 0x174;
    }

    namespace AudioTremolo {
         inline constexpr uintptr_t Duty = 0x124;
         inline constexpr uintptr_t Frequency = 0x128;
         inline constexpr uintptr_t Shape = 0x12c;
         inline constexpr uintptr_t Skew = 0x130;
    }

    namespace AudioWindSynthesizer {
         inline constexpr uintptr_t Enabled = 0x134;
         inline constexpr uintptr_t Volume = 0x130;
    }

    namespace AuroraScript {
         inline constexpr uintptr_t LODCriticality = 0x120;
         inline constexpr uintptr_t Priority = 0x124;
         inline constexpr uintptr_t Source = 0xf0;
    }

    namespace AuroraScriptObject {
         inline constexpr uintptr_t FrameId = 0xb8;
         inline constexpr uintptr_t LODLevel = 0xbc;
         inline constexpr uintptr_t MaxFrequency = 0xc0;
         inline constexpr uintptr_t PriorFrameInvoked = 0xc4;
    }

    namespace AuroraService {
         inline constexpr uintptr_t BufferFullInputCount = 0xc8;
         inline constexpr uintptr_t InputDropRate = 0xcc;
         inline constexpr uintptr_t OutOfOrderInputCount = 0xd0;
         inline constexpr uintptr_t RollbackOffset = 0xd4;
         inline constexpr uintptr_t TooOldInputCount = 0xd8;
    }

    namespace AvatarAbilityRules {
         inline constexpr uintptr_t EnableClimbing = 0x9c;
         inline constexpr uintptr_t EnableCrouching = 0x9d;
         inline constexpr uintptr_t EnableFallingDown = 0x9e;
         inline constexpr uintptr_t EnableHolding = 0xa0;
         inline constexpr uintptr_t EnableJumping = 0xa1;
         inline constexpr uintptr_t EnableSitting = 0xa4;
         inline constexpr uintptr_t EnableSprinting = 0xa5;
    }

    namespace AvatarAccessoryRules {
         inline constexpr uintptr_t BackAccessoryOverride = 0x98;
         inline constexpr uintptr_t CustomBackAccessoryId = 0xa0;
         inline constexpr uintptr_t CustomFaceAccessoryId = 0xa8;
         inline constexpr uintptr_t CustomFrontAccessoryId = 0xb0;
         inline constexpr uintptr_t CustomHairAccessoryId = 0xb8;
         inline constexpr uintptr_t CustomHeadAccessoryId = 0xc0;
         inline constexpr uintptr_t CustomNeckAccessoryId = 0xc8;
         inline constexpr uintptr_t CustomShoulderAccessoryId = 0xd0;
         inline constexpr uintptr_t CustomWaistAccessoryId = 0xd8;
         inline constexpr uintptr_t EnableEmissives = 0x138;
         inline constexpr uintptr_t FaceAccessoryOverride = 0xe0;
         inline constexpr uintptr_t FrontAccessoryOverride = 0xe8;
         inline constexpr uintptr_t HeadAccessoryOverride = 0xf8;
         inline constexpr uintptr_t NeckAccessoryOverride = 0x100;
         inline constexpr uintptr_t ShoulderAccessoryOverride = 0x108;
         inline constexpr uintptr_t WaistAccessoryOverride = 0x110;
    }

    namespace AvatarAnimationRules {
         inline constexpr uintptr_t ClimbAnimationOverride = 0x98;
         inline constexpr uintptr_t CustomClimbAnimationId = 0xa0;
         inline constexpr uintptr_t CustomFallAnimationId = 0xa8;
         inline constexpr uintptr_t CustomIdleAlt1AnimationId = 0xb0;
         inline constexpr uintptr_t CustomIdleAlt2AnimationId = 0xb8;
         inline constexpr uintptr_t CustomIdleAnimationId = 0xc0;
         inline constexpr uintptr_t CustomJumpAnimationId = 0xc8;
         inline constexpr uintptr_t CustomRunAnimationId = 0xd0;
         inline constexpr uintptr_t CustomSwimAnimationId = 0xd8;
         inline constexpr uintptr_t CustomSwimIdleAnimationId = 0xe0;
         inline constexpr uintptr_t CustomWalkAnimationId = 0xe8;
         inline constexpr uintptr_t IdleAltAnimationOverride = 0xf8;
         inline constexpr uintptr_t IdleAnimationOverride = 0x100;
         inline constexpr uintptr_t JumpAnimationOverride = 0x108;
         inline constexpr uintptr_t MoodAnimationOverride = 0x110;
         inline constexpr uintptr_t RunAnimationOverride = 0x118;
         inline constexpr uintptr_t SwimAnimationOverride = 0x120;
         inline constexpr uintptr_t SwimIdleAnimationOverride = 0x128;
         inline constexpr uintptr_t WalkAnimationOverride = 0x130;
    }

    namespace AvatarBodyRules {
         inline constexpr uintptr_t CustomBodyBundleId = 0x98;
         inline constexpr uintptr_t CustomEyebrowEnabled = 0x164;
         inline constexpr uintptr_t CustomEyebrowId = 0xa0;
         inline constexpr uintptr_t CustomEyelashEnabled = 0x165;
         inline constexpr uintptr_t CustomEyelashId = 0xa8;
         inline constexpr uintptr_t CustomFaceEnabled = 0x166;
         inline constexpr uintptr_t CustomFaceId = 0xb0;
         inline constexpr uintptr_t CustomHeadEnabled = 0x167;
         inline constexpr uintptr_t CustomHeadId = 0xb8;
         inline constexpr uintptr_t CustomLeftArmEnabled = 0x168;
         inline constexpr uintptr_t CustomLeftArmId = 0xc0;
         inline constexpr uintptr_t CustomLeftLegId = 0xc8;
         inline constexpr uintptr_t CustomMoodEnabled = 0x16a;
         inline constexpr uintptr_t CustomMoodId = 0xd0;
         inline constexpr uintptr_t CustomRightArmEnabled = 0x16b;
         inline constexpr uintptr_t CustomRightArmId = 0xd8;
         inline constexpr uintptr_t CustomRightLegEnabled = 0x16c;
         inline constexpr uintptr_t CustomRightLegId = 0xe0;
         inline constexpr uintptr_t CustomTorsoEnabled = 0x16d;
         inline constexpr uintptr_t CustomTorsoId = 0xe8;
         inline constexpr uintptr_t KeepPlayerHead = 0x16e;
         inline constexpr uintptr_t LeftArmOverride = 0xf8;
         inline constexpr uintptr_t LeftLegOverride = 0x100;
         inline constexpr uintptr_t RightArmOverride = 0x108;
         inline constexpr uintptr_t RightLegOverride = 0x110;
         inline constexpr uintptr_t TorsoOverride = 0x118;
    }

    namespace AvatarClothingRules {
         inline constexpr uintptr_t ClassicPantsOverride = 0x98;
         inline constexpr uintptr_t ClassicShirtOverride = 0xa0;
         inline constexpr uintptr_t ClassicTShirtOverride = 0xa8;
         inline constexpr uintptr_t CustomClassicPantsAccessoryEnabled = 0x16c;
         inline constexpr uintptr_t CustomClassicPantsAccessoryId = 0xb0;
         inline constexpr uintptr_t CustomClassicShirtsAccessoryEnabled = 0x16d;
         inline constexpr uintptr_t CustomClassicShirtsAccessoryId = 0xb8;
         inline constexpr uintptr_t CustomClassicTShirtsAccessoryEnabled = 0x16e;
         inline constexpr uintptr_t CustomClassicTShirtsAccessoryId = 0xc0;
         inline constexpr uintptr_t CustomDressSkirtAccessoryEnabled = 0x16f;
         inline constexpr uintptr_t CustomDressSkirtAccessoryId = 0xc8;
         inline constexpr uintptr_t CustomJacketAccessoryEnabled = 0x170;
         inline constexpr uintptr_t CustomJacketAccessoryId = 0xd0;
         inline constexpr uintptr_t CustomLeftShoesAccessoryEnabled = 0x171;
         inline constexpr uintptr_t CustomLeftShoesAccessoryId = 0xd8;
         inline constexpr uintptr_t CustomPantsAccessoryEnabled = 0x172;
         inline constexpr uintptr_t CustomPantsAccessoryId = 0xe0;
         inline constexpr uintptr_t CustomRightShoesAccessoryEnabled = 0x173;
         inline constexpr uintptr_t CustomRightShoesAccessoryId = 0xe8;
         inline constexpr uintptr_t CustomShirtAccessoryEnabled = 0x174;
         inline constexpr uintptr_t CustomShortsAccessoryEnabled = 0x175;
         inline constexpr uintptr_t CustomShortsAccessoryId = 0xf8;
         inline constexpr uintptr_t CustomSweaterAccessoryEnabled = 0x176;
         inline constexpr uintptr_t CustomSweaterAccessoryId = 0x100;
         inline constexpr uintptr_t CustomTShirtAccessoryEnabled = 0x177;
         inline constexpr uintptr_t CustomTShirtAccessoryId = 0x108;
         inline constexpr uintptr_t DressSkirtAccessoryOverride = 0x110;
         inline constexpr uintptr_t JacketAccessoryOverride = 0x118;
         inline constexpr uintptr_t LeftShoeAccessoryOverride = 0x120;
         inline constexpr uintptr_t LimitBounds = 0x158;
         inline constexpr uintptr_t PantsAccessoryOverride = 0x128;
         inline constexpr uintptr_t RightShoeAccessoryOverride = 0x130;
         inline constexpr uintptr_t ShirtAccessoryOverride = 0x138;
         inline constexpr uintptr_t ShortsAccessoryOverride = 0x140;
         inline constexpr uintptr_t SweaterAccessoryOverride = 0x148;
    }

    namespace AvatarCollisionRules {
         inline constexpr uintptr_t HitboxSize = 0x98;
    }

    namespace AvatarEffectDescription {
         inline constexpr uintptr_t AssetId = 0x98;
    }

    namespace AvatarSettings {
         inline constexpr uintptr_t RulesSchemaVersion = 0xc8;
    }

    namespace BallSocketConstraint {
         inline constexpr uintptr_t Radius = 0x15c;
         inline constexpr uintptr_t Restitution = 0x160;
         inline constexpr uintptr_t TwistLowerAngle = 0x164;
         inline constexpr uintptr_t TwistUpperAngle = 0x168;
    }

    namespace BaseCoreGuiConfiguration {
         inline constexpr uintptr_t Enabled = 0x98;
    }

    namespace BasePart {
         inline constexpr uintptr_t AudioCanCollide = 0x11f;
         inline constexpr uintptr_t BackParamA = 0xb0;
         inline constexpr uintptr_t BackParamB = 0xb4;
         inline constexpr uintptr_t BottomParamB = 0xc4;
         inline constexpr uintptr_t CastShadow = 0x120;
         inline constexpr uintptr_t Color3 = 0x11c;
         inline constexpr uintptr_t LeftParamA = 0xe0;
         inline constexpr uintptr_t LeftParamB = 0xe4;
         inline constexpr uintptr_t LocalTransparencyModifier = 0xf0;
         inline constexpr uintptr_t Locked = 0x121;
         inline constexpr uintptr_t Massless = 0x122;
         inline constexpr uintptr_t Primitive = 0x178;
         inline constexpr uintptr_t Reflectance = 0xf4;
         inline constexpr uintptr_t RightParamB = 0xfc;
         inline constexpr uintptr_t Shape = 0x1a0;
         inline constexpr uintptr_t TopParamA = 0x108;
         inline constexpr uintptr_t TopParamB = 0x10c;
         inline constexpr uintptr_t Transparency = 0x118;
    }

    namespace BaseScript {
         inline constexpr uintptr_t LinkedSource = 0xf0;
    }

    namespace BaseWrap {
         inline constexpr uintptr_t CageMeshContent = 0xa0;
         inline constexpr uintptr_t CageOrigin = 0x148;
         inline constexpr uintptr_t HSRData = 0x138;
         inline constexpr uintptr_t HSRMeshIdData = 0x140;
         inline constexpr uintptr_t ImportInProcess = 0x1a8;
         inline constexpr uintptr_t ImportOrigin = 0x178;
    }

    namespace Beam {
         inline constexpr uintptr_t Attachment0 = 0x148;
         inline constexpr uintptr_t Attachment1 = 0x158;
         inline constexpr uintptr_t Brightness = 0x168;
         inline constexpr uintptr_t Color = 0xe0;
         inline constexpr uintptr_t CurveSize0 = 0x16c;
         inline constexpr uintptr_t CurveSize1 = 0x170;
         inline constexpr uintptr_t Enabled = 0x19c;
         inline constexpr uintptr_t FaceCamera = 0x19d;
         inline constexpr uintptr_t LightEmission = 0x174;
         inline constexpr uintptr_t LightInfluence = 0x178;
         inline constexpr uintptr_t LocalTransparencyModifier = 0x17c;
         inline constexpr uintptr_t Segments = 0x180;
         inline constexpr uintptr_t Texture = 0x128;
         inline constexpr uintptr_t TextureLength = 0x184;
         inline constexpr uintptr_t TextureSpeed = 0x18c;
         inline constexpr uintptr_t Transparency = 0xa0;
         inline constexpr uintptr_t Width0 = 0x190;
         inline constexpr uintptr_t Width1 = 0x194;
         inline constexpr uintptr_t ZOffset = 0x198;
    }

    namespace BevelMesh {
         inline constexpr uintptr_t Bevel = 0xc8;
         inline constexpr uintptr_t Data = 0xcc;
    }

    namespace BillboardGui {
         inline constexpr uintptr_t Active = 0x58c;
         inline constexpr uintptr_t AlwaysOnTop = 0x58d;
         inline constexpr uintptr_t Brightness = 0x570;
         inline constexpr uintptr_t ClipsDescendants = 0x58e;
         inline constexpr uintptr_t CurrentDistance = 0x574;
         inline constexpr uintptr_t DistanceLowerLimit = 0x578;
         inline constexpr uintptr_t DistanceStep = 0x57c;
         inline constexpr uintptr_t DistanceUpperLimit = 0x580;
         inline constexpr uintptr_t ExtentsOffset = 0x538;
         inline constexpr uintptr_t ExtentsOffsetWorldSpace = 0x544;
         inline constexpr uintptr_t LightInfluence = 0x584;
         inline constexpr uintptr_t MaxDistance = 0x588;
         inline constexpr uintptr_t Size = 0x528;
         inline constexpr uintptr_t StudsOffset = 0x550;
         inline constexpr uintptr_t StudsOffsetWorldSpace = 0x55c;
    }

    namespace BloomEffect {
         inline constexpr uintptr_t Enabled = 0x98;
         inline constexpr uintptr_t Intensity = 0xa0;
         inline constexpr uintptr_t Size = 0xa4;
         inline constexpr uintptr_t Threshold = 0xa8;
    }

    namespace BlurEffect {
         inline constexpr uintptr_t Enabled = 0x98;
         inline constexpr uintptr_t Size = 0xa0;
    }

    namespace BodyAngularVelocity {
         inline constexpr uintptr_t P = 0x288;
    }

    namespace BodyColors {
         inline constexpr uintptr_t HeadColor = 0xa8;
         inline constexpr uintptr_t LeftArmColor3 = 0xbc;
         inline constexpr uintptr_t RightLegColor3 = 0xec;
    }

    namespace BodyForce {
         inline constexpr uintptr_t Force = 0x268;
    }

    namespace BodyGyro {
         inline constexpr uintptr_t D = 0x2ac;
         inline constexpr uintptr_t P = 0x2b0;
    }

    namespace BodyPartDescription {
         inline constexpr uintptr_t AssetId = 0xc8;
    }

    namespace BodyPosition {
         inline constexpr uintptr_t D = 0x288;
         inline constexpr uintptr_t P = 0x28c;
    }

    namespace BodyThrust {
         inline constexpr uintptr_t Force = 0x268;
    }

    namespace BodyVelocity {
         inline constexpr uintptr_t P = 0x288;
    }

    namespace Bone {
         inline constexpr uintptr_t Transform = 0x168;
    }

    namespace BoolValue {
         inline constexpr uintptr_t Value = 0xa0;
    }

    namespace BoxHandleAdornment {
         inline constexpr uintptr_t DoubleSided = 0x1a0;
         inline constexpr uintptr_t Size = 0x190;
    }

    namespace BrickColorValue {
         inline constexpr uintptr_t Value = 0xa0;
    }

    namespace BubbleChatConfiguration {
         inline constexpr uintptr_t BackgroundColor3 = 0xd8;
         inline constexpr uintptr_t BubbleDuration = 0xfc;
         inline constexpr uintptr_t BubblesSpacing = 0x100;
         inline constexpr uintptr_t LocalPlayerStudsOffset = 0xe4;
         inline constexpr uintptr_t MaxBubbles = 0x104;
         inline constexpr uintptr_t MaxDistance = 0x108;
         inline constexpr uintptr_t MinimizeDistance = 0x10c;
         inline constexpr uintptr_t TextColor3 = 0xf0;
         inline constexpr uintptr_t TextSize = 0xd0;
         inline constexpr uintptr_t VerticalStudsOffset = 0x110;
    }

    namespace BubbleChatMessageProperties {
         inline constexpr uintptr_t TailVisible = 0x128;
         inline constexpr uintptr_t TextSize = 0x108;
    }

    namespace ByteCode {
         inline constexpr uintptr_t Pointer = 0x18;
         inline constexpr uintptr_t Size = 0x28;
    }

    namespace CachedItem {
         inline constexpr uintptr_t FileMeshData = 0x28;
    }

    namespace Camera {
         inline constexpr uintptr_t CameraSubject = 0xb0;
         inline constexpr uintptr_t CameraType = 0x120;
         inline constexpr uintptr_t FieldOfView = 0x128;
         inline constexpr uintptr_t Focus = 0xf0;
         inline constexpr uintptr_t HeadScale = 0x130;
         inline constexpr uintptr_t ImagePlaneDepth = 0x2d4;
         inline constexpr uintptr_t NearPlaneZ = 0x138;
         inline constexpr uintptr_t OrthographicSize = 0x140;
         inline constexpr uintptr_t Position = 0xe4;
         inline constexpr uintptr_t Rotation = 0xc0;
         inline constexpr uintptr_t ViewStretch = 0x148;
         inline constexpr uintptr_t Viewport = 0x284;
         inline constexpr uintptr_t ViewportSize = 0x2c4;
    }

    namespace CanvasGroup {
         inline constexpr uintptr_t ResolutionScale = 0x670;
    }

    namespace CapturesViewConfiguration {
         inline constexpr uintptr_t Open = 0xa0;
    }

    namespace ChannelSelectorSoundEffect {
         inline constexpr uintptr_t Channel = 0xe8;
    }

    namespace ChannelTabsConfiguration {
         inline constexpr uintptr_t BackgroundColor3 = 0xc0;
         inline constexpr uintptr_t BackgroundTransparency = 0xa0;
         inline constexpr uintptr_t Enabled = 0x10c;
         inline constexpr uintptr_t HoverBackgroundColor3 = 0xcc;
         inline constexpr uintptr_t SelectedTabTextColor3 = 0xd8;
         inline constexpr uintptr_t TextColor3 = 0xe4;
         inline constexpr uintptr_t TextSize = 0xb0;
         inline constexpr uintptr_t TextStrokeColor3 = 0xf0;
    }

    namespace CharacterMesh {
         inline constexpr uintptr_t BaseTextureId = 0xb0;
         inline constexpr uintptr_t BodyPart = 0x130;
         inline constexpr uintptr_t MeshId = 0xe0;
         inline constexpr uintptr_t OverlayTextureId = 0x110;
    }

    namespace Chat {
         inline constexpr uintptr_t BubbleChatEnabled = 0x118;
         inline constexpr uintptr_t IsAutoMigrated = 0x119;
    }

    namespace ChatInputBarConfiguration {
         inline constexpr uintptr_t AbsolutePosition = 0x110;
         inline constexpr uintptr_t AutocompleteEnabled = 0x124;
         inline constexpr uintptr_t Enabled = 0x125;
         inline constexpr uintptr_t IsFocused = 0x126;
         inline constexpr uintptr_t TextSize = 0xd0;
         inline constexpr uintptr_t TextStrokeTransparency = 0xd8;
    }

    namespace ChatWindowConfiguration {
         inline constexpr uintptr_t BackgroundColor3 = 0xc0;
         inline constexpr uintptr_t BackgroundTransparency = 0xa0;
         inline constexpr uintptr_t Enabled = 0x108;
         inline constexpr uintptr_t HeightScale = 0xf4;
         inline constexpr uintptr_t TextColor3 = 0xcc;
         inline constexpr uintptr_t TextSize = 0xb0;
         inline constexpr uintptr_t TextStrokeColor3 = 0xd8;
         inline constexpr uintptr_t WidthScale = 0x104;
    }

    namespace ChatWindowMessageProperties {
         inline constexpr uintptr_t TextColor3 = 0x118;
         inline constexpr uintptr_t TextSize = 0x130;
    }

    namespace ChorusSoundEffect {
         inline constexpr uintptr_t Mix = 0xe4;
         inline constexpr uintptr_t Rate = 0xe8;
    }

    namespace ClickDetector {
         inline constexpr uintptr_t MaxActivationDistance = 0xd0;
         inline constexpr uintptr_t MouseIcon = 0xb0;
    }

    namespace ClimbController {
         inline constexpr uintptr_t BalanceMaxTorque = 0xcc;
         inline constexpr uintptr_t BalanceSpeed = 0xd0;
         inline constexpr uintptr_t MoveMaxForce = 0xd4;
    }

    namespace Clothing {
         inline constexpr uintptr_t Color3 = 0x108;
         inline constexpr uintptr_t Template = 0xe8;
    }

    namespace CloudClientService {
         inline constexpr uintptr_t IsCloudClient = 0xb8;
         inline constexpr uintptr_t RobloxRealityFilterEnabled = 0xba;
         inline constexpr uintptr_t SessionTimeLimit = 0xb0;
    }

    namespace Clouds {
         inline constexpr uintptr_t Cover = 0xa4;
         inline constexpr uintptr_t Density = 0xa8;
    }

    namespace ColorCorrectionEffect {
         inline constexpr uintptr_t Brightness = 0xac;
         inline constexpr uintptr_t Contrast = 0xb0;
         inline constexpr uintptr_t Enabled = 0x98;
         inline constexpr uintptr_t Saturation = 0xb4;
         inline constexpr uintptr_t TintColor = 0xa0;
    }

    namespace ColorGradingEffect {
         inline constexpr uintptr_t Enabled = 0x98;
         inline constexpr uintptr_t TonemapperPreset = 0xa0;
    }

    namespace CompressorSoundEffect {
         inline constexpr uintptr_t Attack = 0xf0;
         inline constexpr uintptr_t Ratio = 0xf8;
         inline constexpr uintptr_t Release = 0xfc;
         inline constexpr uintptr_t Threshold = 0x100;
    }

    namespace ConeHandleAdornment {
         inline constexpr uintptr_t DoubleSided = 0x19c;
         inline constexpr uintptr_t Height = 0x190;
         inline constexpr uintptr_t Hollow = 0x19d;
         inline constexpr uintptr_t Radius = 0x194;
    }

    namespace Constraint {
         inline constexpr uintptr_t Enabled = 0xa1;
    }

    namespace ControllerBase {
         inline constexpr uintptr_t Active = 0xa4;
         inline constexpr uintptr_t BalanceRigidityEnabled = 0xa5;
    }

    namespace ControllerManager {
         inline constexpr uintptr_t BaseMoveSpeed = 0x104;
         inline constexpr uintptr_t BaseTurnSpeed = 0x108;
         inline constexpr uintptr_t FacingDirection = 0xe0;
         inline constexpr uintptr_t UpDirection = 0xf8;
    }

    namespace ControllerPartSensor {
         inline constexpr uintptr_t LadderSearchHeight = 0x274;
         inline constexpr uintptr_t LadderSearchOffset = 0x278;
         inline constexpr uintptr_t SearchDistance = 0x27c;
    }

    namespace CustomEvent {
         inline constexpr uintptr_t PersistedCurrentValue = 0x98;
    }

    namespace CylinderHandleAdornment {
         inline constexpr uintptr_t Angle = 0x190;
         inline constexpr uintptr_t Height = 0x194;
         inline constexpr uintptr_t Radius = 0x19c;
    }

    namespace CylindricalConstraint {
         inline constexpr uintptr_t AngularResponsiveness = 0x1bc;
         inline constexpr uintptr_t AngularRestitution = 0x1c0;
         inline constexpr uintptr_t AngularSpeed = 0x1c4;
         inline constexpr uintptr_t AngularVelocity = 0x1c8;
         inline constexpr uintptr_t LowerAngle = 0x1d0;
         inline constexpr uintptr_t MotorMaxAngularAcceleration = 0x1d4;
         inline constexpr uintptr_t MotorMaxTorque = 0x1d8;
         inline constexpr uintptr_t ServoMaxTorque = 0x1dc;
         inline constexpr uintptr_t SoftlockAngularServoUponReachingTarget = 0x1ea;
         inline constexpr uintptr_t TargetAngle = 0x1e0;
         inline constexpr uintptr_t UpperAngle = 0x1e4;
    }

    namespace DataModel {
         inline constexpr uintptr_t CreatorId = 0x190;
         inline constexpr uintptr_t ForceR15 = 0x1c0;
         inline constexpr uintptr_t GameId = 0x198;
         inline constexpr uintptr_t GameLoaded = 0x5b8;
         inline constexpr uintptr_t JobId = 0x108;
         inline constexpr uintptr_t PlaceId = 0x1a0;
         inline constexpr uintptr_t PlaceVersion = 0x1bc;
         inline constexpr uintptr_t PrimitiveCount = 0x430;
         inline constexpr uintptr_t ScriptContext = 0x440;
         inline constexpr uintptr_t ServerIP = 0x5a0;
         inline constexpr uintptr_t ToRenderView1 = 0x1d8;
         inline constexpr uintptr_t ToRenderView2 = 0x8;
         inline constexpr uintptr_t ToRenderView3 = 0x28;
         inline constexpr uintptr_t Workspace = 0x168;
    }

    namespace DataModelMesh {
         inline constexpr uintptr_t Offset = 0xa0;
         inline constexpr uintptr_t Scale = 0xac;
         inline constexpr uintptr_t VertexColor = 0xb8;
    }

    namespace DataStoreGetOptions {
         inline constexpr uintptr_t UseCache = 0x98;
    }

    namespace DataStoreInfo {
         inline constexpr uintptr_t CreatedTime = 0xb8;
         inline constexpr uintptr_t UpdatedTime = 0xc0;
    }

    namespace DataStoreKeyInfo {
         inline constexpr uintptr_t CreatedTime = 0xb8;
         inline constexpr uintptr_t UpdatedTime = 0xc0;
    }

    namespace DataStoreObjectVersionInfo {
         inline constexpr uintptr_t CreatedTime = 0xb8;
         inline constexpr uintptr_t IsDeleted = 0xc0;
    }

    namespace DataStoreOptions {
         inline constexpr uintptr_t AllScopes = 0x98;
    }

    namespace DataStoreService {
         inline constexpr uintptr_t AutomaticRetry = 0xb0;
         inline constexpr uintptr_t LegacyNamingScheme = 0xb1;
    }

    namespace Debris {
         inline constexpr uintptr_t MaxItems = 0xb0;
    }

    namespace DebuggerBreakpoint {
         inline constexpr uintptr_t ContinueExecution = 0xa4;
         inline constexpr uintptr_t IsEnabled = 0xa5;
         inline constexpr uintptr_t Line = 0xa0;
    }

    namespace DebuggerManager {
         inline constexpr uintptr_t DebuggingEnabled = 0xa8;
    }

    namespace Decal {
         inline constexpr uintptr_t AutoLocalize = 0x280;
         inline constexpr uintptr_t EmissiveMaskContent = 0xc8;
         inline constexpr uintptr_t EmissiveStrength = 0x260;
         inline constexpr uintptr_t LocalTransparencyModifier = 0x264;
         inline constexpr uintptr_t Rotation = 0x26c;
         inline constexpr uintptr_t Shiny = 0x270;
         inline constexpr uintptr_t Specular = 0x274;
         inline constexpr uintptr_t TexturePackContent = 0x1e8;
         inline constexpr uintptr_t Transparency = 0x278;
         inline constexpr uintptr_t ZIndex = 0x27c;
    }

    namespace DeferredAssetManagerService {
         inline constexpr uintptr_t JoiningPlaceId = 0xb0;
         inline constexpr uintptr_t JoiningUniverseId = 0xb8;
         inline constexpr uintptr_t PregameLoadingScreenOnly = 0xc0;
    }

    namespace DepthOfFieldEffect {
         inline constexpr uintptr_t Enabled = 0x98;
         inline constexpr uintptr_t FarIntensity = 0xa0;
         inline constexpr uintptr_t FocusDistance = 0xa4;
         inline constexpr uintptr_t InFocusRadius = 0xa8;
         inline constexpr uintptr_t NearIntensity = 0xac;
    }

    namespace Dialog {
         inline constexpr uintptr_t ConversationDistance = 0xe8;
         inline constexpr uintptr_t GoodbyeChoiceActive = 0xf8;
         inline constexpr uintptr_t InUse = 0xf9;
         inline constexpr uintptr_t TriggerDistance = 0xf4;
         inline constexpr uintptr_t TriggerOffset = 0xd8;
    }

    namespace DialogChoice {
         inline constexpr uintptr_t GoodbyeChoiceActive = 0x100;
    }

    namespace DockWidgetPluginGui {
         inline constexpr uintptr_t HostWidgetWasRestored = 0x638;
    }

    namespace DoubleConstrainedValue {
         inline constexpr uintptr_t MaxValue = 0xa0;
    }

    namespace DragDetector {
         inline constexpr uintptr_t ActivatedCursorIcon = 0x188;
         inline constexpr uintptr_t CursorIcon = 0xb0;
         inline constexpr uintptr_t MaxActivationDistance = 0xd0;
         inline constexpr uintptr_t MaxDragAngle = 0x270;
         inline constexpr uintptr_t MaxDragTranslation = 0x234;
         inline constexpr uintptr_t MaxForce = 0x274;
         inline constexpr uintptr_t MaxTorque = 0x278;
         inline constexpr uintptr_t MinDragAngle = 0x27c;
         inline constexpr uintptr_t MinDragTranslation = 0x240;
         inline constexpr uintptr_t ReferenceInstance = 0x1b8;
         inline constexpr uintptr_t Responsiveness = 0x288;
    }

    namespace EchoSoundEffect {
         inline constexpr uintptr_t DryLevel = 0xe4;
         inline constexpr uintptr_t Feedback = 0xe8;
         inline constexpr uintptr_t WetLevel = 0xec;
    }

    namespace EditableImage {
         inline constexpr uintptr_t IsReplicatedCopy = 0xc0;
    }

    namespace EditableMesh {
         inline constexpr uintptr_t FixedSize = 0xc0;
         inline constexpr uintptr_t IsReplicatedCopy = 0xc1;
    }

    namespace EqualizerSoundEffect {
         inline constexpr uintptr_t LowGain = 0xe4;
         inline constexpr uintptr_t MidGain = 0xe8;
    }

    namespace ExperienceInviteOptions {
         inline constexpr uintptr_t InviteUser = 0xf8;
    }

    namespace ExperienceStateCaptureService {
         inline constexpr uintptr_t HiddenSelectionEnabled = 0xac;
         inline constexpr uintptr_t IsInBackground = 0xad;
         inline constexpr uintptr_t IsInCaptureMode = 0xae;
    }

    namespace ExperienceStateRecordingService {
         inline constexpr uintptr_t IsServerDataModelRecorderActive = 0xb0;
    }

    namespace Explosion {
         inline constexpr uintptr_t BlastPressure = 0xe4;
         inline constexpr uintptr_t BlastRadius = 0xe8;
         inline constexpr uintptr_t DestroyJointRadiusPercent = 0xec;
         inline constexpr uintptr_t TimeScale = 0xf8;
    }

    namespace FaceControls {
         inline constexpr uintptr_t ChinRaiser = 0xc8;
         inline constexpr uintptr_t ChinRaiserUpperLip = 0xcc;
         inline constexpr uintptr_t Corrugator = 0xd0;
         inline constexpr uintptr_t EyesLookDown = 0xd4;
         inline constexpr uintptr_t EyesLookLeft = 0xd8;
         inline constexpr uintptr_t EyesLookRight = 0xdc;
         inline constexpr uintptr_t EyesLookUp = 0xe0;
         inline constexpr uintptr_t FlatPucker = 0xe4;
         inline constexpr uintptr_t Funneler = 0xe8;
         inline constexpr uintptr_t InternalOverrideFACSData = 0xa0;
         inline constexpr uintptr_t JawDrop = 0xec;
         inline constexpr uintptr_t JawLeft = 0xf0;
         inline constexpr uintptr_t JawRight = 0xf4;
         inline constexpr uintptr_t LeftBrowLowerer = 0xf8;
         inline constexpr uintptr_t LeftCheekPuff = 0xfc;
         inline constexpr uintptr_t LeftCheekRaiser = 0x100;
         inline constexpr uintptr_t LeftDimpler = 0x104;
         inline constexpr uintptr_t LeftEyeClosed = 0x108;
         inline constexpr uintptr_t LeftEyeUpperLidRaiser = 0x10c;
         inline constexpr uintptr_t LeftInnerBrowRaiser = 0x110;
         inline constexpr uintptr_t LeftLipCornerDown = 0x114;
         inline constexpr uintptr_t LeftLipCornerPuller = 0x118;
         inline constexpr uintptr_t LeftLipStretcher = 0x11c;
         inline constexpr uintptr_t LeftLowerLipDepressor = 0x120;
         inline constexpr uintptr_t LeftNoseWrinkler = 0x124;
         inline constexpr uintptr_t LeftOuterBrowRaiser = 0x128;
         inline constexpr uintptr_t LeftUpperLipRaiser = 0x12c;
         inline constexpr uintptr_t LipPresser = 0x130;
         inline constexpr uintptr_t LipsTogether = 0x134;
         inline constexpr uintptr_t LowerLipSuck = 0x138;
         inline constexpr uintptr_t MouthLeft = 0x13c;
         inline constexpr uintptr_t MouthRight = 0x140;
         inline constexpr uintptr_t Pucker = 0x144;
         inline constexpr uintptr_t RightBrowLowerer = 0x148;
         inline constexpr uintptr_t RightCheekPuff = 0x14c;
         inline constexpr uintptr_t RightCheekRaiser = 0x150;
         inline constexpr uintptr_t RightDimpler = 0x154;
         inline constexpr uintptr_t RightEyeClosed = 0x158;
         inline constexpr uintptr_t RightEyeUpperLidRaiser = 0x15c;
         inline constexpr uintptr_t RightInnerBrowRaiser = 0x160;
         inline constexpr uintptr_t RightLipCornerDown = 0x164;
         inline constexpr uintptr_t RightLipCornerPuller = 0x168;
         inline constexpr uintptr_t RightLipStretcher = 0x16c;
         inline constexpr uintptr_t RightLowerLipDepressor = 0x170;
         inline constexpr uintptr_t RightNoseWrinkler = 0x174;
         inline constexpr uintptr_t RightOuterBrowRaiser = 0x178;
         inline constexpr uintptr_t RightUpperLipRaiser = 0x17c;
         inline constexpr uintptr_t TongueDown = 0x180;
         inline constexpr uintptr_t TongueOut = 0x184;
         inline constexpr uintptr_t TongueUp = 0x188;
         inline constexpr uintptr_t UpperLipSuck = 0x18c;
    }

    namespace FakeDataModel {
         inline constexpr uintptr_t Pointer = 0x8f8e968;
         inline constexpr uintptr_t RealDataModel = 0x1f8;
    }

    namespace FeatureMatchingTable {
         inline constexpr uintptr_t Data = 0xc0;
    }

    namespace FeatureSearchParams {
         inline constexpr uintptr_t AnchorCostScale = 0x68;
         inline constexpr uintptr_t AnchorIgnoresTagFilters = 0x80;
         inline constexpr uintptr_t AnchorRow = 0x6c;
         inline constexpr uintptr_t MaxDistance = 0x70;
         inline constexpr uintptr_t MaxResults = 0x74;
         inline constexpr uintptr_t NormalizationKey = 0x58;
         inline constexpr uintptr_t ReturnChannelCosts = 0x81;
         inline constexpr uintptr_t SelectionSeed = 0x60;
         inline constexpr uintptr_t SelectionTemperature = 0x7c;
    }

    namespace FileMesh {
         inline constexpr uintptr_t MeshId = 0xe0;
    }

    namespace FileMeshData {
         inline constexpr uintptr_t AABBMax = 0x18c;
         inline constexpr uintptr_t AABBMin = 0x180;
         inline constexpr uintptr_t Faces = 0x30;
         inline constexpr uintptr_t FacesEnd = 0x38;
         inline constexpr uintptr_t Vertices = 0x0;
         inline constexpr uintptr_t VerticesEnd = 0x8;
    }

    namespace Fire {
         inline constexpr uintptr_t Enabled = 0xc8;
         inline constexpr uintptr_t Heat = 0xc0;
         inline constexpr uintptr_t LocalTransparencyModifier = 0xb8;
         inline constexpr uintptr_t Size = 0xc4;
         inline constexpr uintptr_t TimeScale = 0xbc;
    }

    namespace FlangeSoundEffect {
         inline constexpr uintptr_t Mix = 0xe4;
         inline constexpr uintptr_t Rate = 0xe8;
    }

    namespace FloorWire {
         inline constexpr uintptr_t CycleOffset = 0x130;
         inline constexpr uintptr_t StudsBetweenTextures = 0x134;
         inline constexpr uintptr_t TextureSize = 0x128;
         inline constexpr uintptr_t Velocity = 0x138;
         inline constexpr uintptr_t WireRadius = 0x13c;
    }

    namespace FollowRotationBoneBehavior {
         inline constexpr uintptr_t Weight = 0xb0;
    }

    namespace FollowTwistBoneBehavior {
         inline constexpr uintptr_t CounterRotate = 0xdc;
         inline constexpr uintptr_t Weight = 0xd8;
    }

    namespace ForceField {
         inline constexpr uintptr_t Visible = 0xa0;
    }

    namespace FriendsCallingInstance {
         inline constexpr uintptr_t ConversationId = 0xb8;
         inline constexpr uintptr_t InitiatorUserId = 0xc0;
         inline constexpr uintptr_t IsDeafened = 0xd4;
         inline constexpr uintptr_t Volume = 0xd0;
    }

    namespace FriendsCallingParticipant {
         inline constexpr uintptr_t IsLocalMuted = 0xb0;
         inline constexpr uintptr_t IsSpeaking = 0xb1;
         inline constexpr uintptr_t UserId = 0x98;
         inline constexpr uintptr_t Volume = 0xac;
    }

    namespace FunctionalTest {
         inline constexpr uintptr_t AllowSleep = 0xc0;
         inline constexpr uintptr_t HasMigratedSettingsToTestService = 0xc1;
    }

    namespace GamepadService {
         inline constexpr uintptr_t GamepadCursorEnabled = 0xb0;
    }

    namespace GetTextBoundsParams {
         inline constexpr uintptr_t Font = 0xc0;
         inline constexpr uintptr_t Text = 0xa0;
         inline constexpr uintptr_t Width = 0xcc;
    }

    namespace Glue {
         inline constexpr uintptr_t F0 = 0x170;
    }

    namespace GroundController {
         inline constexpr uintptr_t AccelerationTime = 0xcc;
         inline constexpr uintptr_t BalanceMaxTorque = 0xd0;
         inline constexpr uintptr_t BalanceSpeed = 0xd4;
         inline constexpr uintptr_t DecelerationTime = 0xd8;
         inline constexpr uintptr_t FrictionWeight = 0xe0;
         inline constexpr uintptr_t GroundOffset = 0xe4;
         inline constexpr uintptr_t StandForce = 0xe8;
         inline constexpr uintptr_t StandSpeed = 0xec;
         inline constexpr uintptr_t TurnSpeedFactor = 0xf0;
    }

    namespace GuiBase2D {
         inline constexpr uintptr_t AbsolutePosition = 0xf4;
         inline constexpr uintptr_t AbsoluteRotation = 0xd0;
         inline constexpr uintptr_t AbsoluteSize = 0xfc;
         inline constexpr uintptr_t IsNotOccluded = 0x11d;
         inline constexpr uintptr_t SelectionGroup = 0x11e;
         inline constexpr uintptr_t TotalGroupScale = 0x118;
    }

    namespace GuiBase3d {
         inline constexpr uintptr_t Transparency = 0xcc;
         inline constexpr uintptr_t Visible = 0xd0;
    }

    namespace GuiButton {
         inline constexpr uintptr_t AutoButtonColor = 0x6ac;
         inline constexpr uintptr_t Modal = 0x6ad;
         inline constexpr uintptr_t MouseButton1ClickConnectionCount = 0x690;
         inline constexpr uintptr_t MouseButton1DownConnectionCount = 0x694;
         inline constexpr uintptr_t MouseButton1UpConnectionCount = 0x698;
         inline constexpr uintptr_t MouseButton2ClickConnectionCount = 0x69c;
         inline constexpr uintptr_t MouseButton2DownConnectionCount = 0x6a0;
         inline constexpr uintptr_t MouseButton2UpConnectionCount = 0x6a4;
         inline constexpr uintptr_t Selected = 0x6ae;
    }

    namespace GuiObject {
         inline constexpr uintptr_t Active = 0x340;
         inline constexpr uintptr_t AnchorPoint = 0x2f0;
         inline constexpr uintptr_t BackgroundColor3 = 0x2d8;
         inline constexpr uintptr_t BackgroundTransparency = 0x2e4;
         inline constexpr uintptr_t BorderColor3 = 0x2e4;
         inline constexpr uintptr_t DragBeginConnectionCount = 0x308;
         inline constexpr uintptr_t DragStoppedConnectionCount = 0x30c;
         inline constexpr uintptr_t Draggable = 0x342;
         inline constexpr uintptr_t Image = 0x670;
         inline constexpr uintptr_t Interactable = 0x343;
         inline constexpr uintptr_t LayoutOrder = 0x314;
         inline constexpr uintptr_t MouseEnterConnectionCount = 0x318;
         inline constexpr uintptr_t MouseLeaveConnectionCount = 0x31c;
         inline constexpr uintptr_t MouseMovedConnectionCount = 0x320;
         inline constexpr uintptr_t MouseWheelBackwardConnectionCount = 0x324;
         inline constexpr uintptr_t MouseWheelForwardConnectionCount = 0x328;
         inline constexpr uintptr_t Position = 0x2a8;
         inline constexpr uintptr_t RichText = 0x7c8;
         inline constexpr uintptr_t Rotation = 0xd0;
         inline constexpr uintptr_t ScreenGui_Enabled = 0x40d;
         inline constexpr uintptr_t Selectable = 0x344;
         inline constexpr uintptr_t Size = 0x2c8;
         inline constexpr uintptr_t Text = 0xa30;
         inline constexpr uintptr_t TextColor3 = 0xae0;
         inline constexpr uintptr_t Visible = 0x72d;
         inline constexpr uintptr_t ZIndex = 0x33c;
    }

    namespace GuiService {
         inline constexpr uintptr_t CoreUISafeAreaInsetBottomRight = 0x100;
         inline constexpr uintptr_t CoreUISafeAreaInsetTopLeft = 0x108;
         inline constexpr uintptr_t FullscreenViewport = 0xd0;
         inline constexpr uintptr_t GuiNavigationEnabled = 0x134;
         inline constexpr uintptr_t HardwareSafeAreaInsetBottomRight = 0x110;
         inline constexpr uintptr_t HardwareSafeAreaInsetTopLeft = 0x118;
         inline constexpr uintptr_t MenuIsOpen = 0x135;
         inline constexpr uintptr_t ScreenContextRawScreenScale = 0x12c;
         inline constexpr uintptr_t ScreenContextTopbarInset = 0xe0;
         inline constexpr uintptr_t TouchControlsEnabled = 0x136;
         inline constexpr uintptr_t ViewportSizeInMM = 0x120;
    }

    namespace HandleAdornment {
         inline constexpr uintptr_t AlwaysOnTop = 0x144;
         inline constexpr uintptr_t CFrame = 0x100;
    }

    namespace Handles {
         inline constexpr uintptr_t MouseButton1DownConnectionCount = 0x10c;
         inline constexpr uintptr_t MouseButton1UpConnectionCount = 0x110;
         inline constexpr uintptr_t MouseDragConnectionCount = 0x114;
         inline constexpr uintptr_t MouseEnterConnectionCount = 0x118;
         inline constexpr uintptr_t MouseLeaveConnectionCount = 0x11c;
    }

    namespace HapticEffect {
         inline constexpr uintptr_t Looped = 0xd4;
         inline constexpr uintptr_t Position = 0xc0;
         inline constexpr uintptr_t Radius = 0xcc;
    }

    namespace HiddenSurfaceRemovalAsset {
         inline constexpr uintptr_t HSRMeshIdData = 0xb8;
    }

    namespace Highlight {
         inline constexpr uintptr_t FillTransparency = 0xcc;
         inline constexpr uintptr_t LineThickness = 0xd0;
         inline constexpr uintptr_t OutlineTransparency = 0xd4;
    }

    namespace HingeConstraint {
         inline constexpr uintptr_t AngularResponsiveness = 0x15c;
         inline constexpr uintptr_t AngularSpeed = 0x160;
         inline constexpr uintptr_t AngularVelocity = 0x164;
         inline constexpr uintptr_t LowerAngle = 0x168;
         inline constexpr uintptr_t MotorMaxTorque = 0x170;
         inline constexpr uintptr_t Radius = 0x174;
         inline constexpr uintptr_t Restitution = 0x178;
         inline constexpr uintptr_t ServoMaxTorque = 0x17c;
         inline constexpr uintptr_t TargetAngle = 0x180;
         inline constexpr uintptr_t UpperAngle = 0x184;
    }

    namespace HopperBin {
         inline constexpr uintptr_t Active = 0x454;
    }

    namespace HttpService {
         inline constexpr uintptr_t HttpEnabled = 0xa8;
    }

    namespace Humanoid {
         inline constexpr uintptr_t AutoJumpEnabled = 0x1cc;
         inline constexpr uintptr_t AutoRotate = 0x1cd;
         inline constexpr uintptr_t AutomaticScalingEnabled = 0x1ce;
         inline constexpr uintptr_t BreakJointsOnDeath = 0x1d0;
         inline constexpr uintptr_t CameraMaxDistance = 0x164;
         inline constexpr uintptr_t CameraMinDistance = 0x168;
         inline constexpr uintptr_t CameraOffset = 0x110;
         inline constexpr uintptr_t DisplayDistanceType = 0x174;
         inline constexpr uintptr_t DisplayName = 0xa0;
         inline constexpr uintptr_t EvaluateStateMachine = 0x1d1;
         inline constexpr uintptr_t FinishedState = 0x1d2;
         inline constexpr uintptr_t FloorMaterial = 0x178;
         inline constexpr uintptr_t Health = 0x184;
         inline constexpr uintptr_t HealthDisplayDistance = 0x17c;
         inline constexpr uintptr_t HealthDisplayType = 0x180;
         inline constexpr uintptr_t Health_XML = 0x184;
         inline constexpr uintptr_t HipHeight = 0x188;
         inline constexpr uintptr_t HumanoidRootPart = 0x468;
         inline constexpr uintptr_t HumanoidState = 0x8b0;
         inline constexpr uintptr_t HumanoidStateID = 0x20;
         inline constexpr uintptr_t InternalAnimationHipHeight = 0x18c;
         inline constexpr uintptr_t InternalAvatarRulesRootPartSize = 0x11c;
         inline constexpr uintptr_t InternalAvatarRulesRootPartSizeScalesWithBody = 0x1d3;
         inline constexpr uintptr_t InternalBodyScale = 0x128;
         inline constexpr uintptr_t InternalHeadScale = 0x190;
         inline constexpr uintptr_t InternalOriginalHipHeight = 0x194;
         inline constexpr uintptr_t IsWalking = 0xa46;
         inline constexpr uintptr_t Jump = 0x1d4;
         inline constexpr uintptr_t JumpHeight = 0x198;
         inline constexpr uintptr_t JumpPower = 0x19c;
         inline constexpr uintptr_t MaxHealth = 0x1a0;
         inline constexpr uintptr_t MaxSlopeAngle = 0x1a4;
         inline constexpr uintptr_t MoveDirection = 0x134;
         inline constexpr uintptr_t MoveToPart = 0x100;
         inline constexpr uintptr_t MoveToPoint = 0x158;
         inline constexpr uintptr_t NameDisplayDistance = 0x1a8;
         inline constexpr uintptr_t NameOcclusion = 0x1ac;
         inline constexpr uintptr_t OverrideDefaultCollisions = 0x1d5;
         inline constexpr uintptr_t PlatformStand = 0x1d6;
         inline constexpr uintptr_t PlatformStatePointer = 0x0;
         inline constexpr uintptr_t RequiresNeck = 0x1d7;
         inline constexpr uintptr_t RigType = 0x1b8;
         inline constexpr uintptr_t SeatPart = 0xf0;
         inline constexpr uintptr_t Sit = 0x1d7;
         inline constexpr uintptr_t Strafe = 0x1d9;
         inline constexpr uintptr_t TargetPoint = 0x140;
         inline constexpr uintptr_t UseJumpPower = 0x1da;
         inline constexpr uintptr_t WalkDirection = 0x14c;
         inline constexpr uintptr_t WalkSpeed = 0x1c8;
         inline constexpr uintptr_t WalkTimer = 0x3f0;
         inline constexpr uintptr_t WalkToPoint = 0x158;
         inline constexpr uintptr_t Walkspeed = 0x1c8;
         inline constexpr uintptr_t WalkspeedCheck = 0x3a4;
    }

    namespace HumanoidDescription {
         inline constexpr uintptr_t BodyTypeScale = 0x120;
         inline constexpr uintptr_t ClimbAnimation = 0xc0;
         inline constexpr uintptr_t DepthScale = 0x124;
         inline constexpr uintptr_t Face = 0xc8;
         inline constexpr uintptr_t FallAnimation = 0xd0;
         inline constexpr uintptr_t GraphicTShirt = 0xd8;
         inline constexpr uintptr_t HeadScale = 0x128;
         inline constexpr uintptr_t HeightScale = 0x12c;
         inline constexpr uintptr_t IdleAnimation = 0xe0;
         inline constexpr uintptr_t JumpAnimation = 0xe8;
         inline constexpr uintptr_t NumberEmotesLoaded = 0x130;
         inline constexpr uintptr_t Pants = 0xf8;
         inline constexpr uintptr_t ProportionScale = 0x134;
         inline constexpr uintptr_t RunAnimation = 0x100;
         inline constexpr uintptr_t Shirt = 0x108;
         inline constexpr uintptr_t SwimAnimation = 0x110;
         inline constexpr uintptr_t WalkAnimation = 0x118;
         inline constexpr uintptr_t WidthScale = 0x138;
    }

    namespace HumanoidRigDescription {
         inline constexpr uintptr_t ChestRangeMax = 0x668;
         inline constexpr uintptr_t ChestRangeMin = 0x674;
         inline constexpr uintptr_t ChestSize = 0x878;
         inline constexpr uintptr_t ChestTposeAdjustment = 0x218;
         inline constexpr uintptr_t HeadBaseRangeMax = 0x680;
         inline constexpr uintptr_t HeadBaseRangeMin = 0x68c;
         inline constexpr uintptr_t HeadBaseSize = 0x87c;
         inline constexpr uintptr_t HeadBaseTposeAdjustment = 0x248;
         inline constexpr uintptr_t LeftAnkleRangeMax = 0x698;
         inline constexpr uintptr_t LeftAnkleRangeMin = 0x6a4;
         inline constexpr uintptr_t LeftAnkleSize = 0x880;
         inline constexpr uintptr_t LeftAnkleTposeAdjustment = 0x278;
         inline constexpr uintptr_t LeftClavicleRangeMax = 0x6b0;
         inline constexpr uintptr_t LeftClavicleRangeMin = 0x6bc;
         inline constexpr uintptr_t LeftClavicleSize = 0x884;
         inline constexpr uintptr_t LeftClavicleTposeAdjustment = 0x2a8;
         inline constexpr uintptr_t LeftElbowRangeMax = 0x6c8;
         inline constexpr uintptr_t LeftElbowRangeMin = 0x6d4;
         inline constexpr uintptr_t LeftElbowSize = 0x888;
         inline constexpr uintptr_t LeftElbowTposeAdjustment = 0x2d8;
         inline constexpr uintptr_t LeftHipRangeMax = 0x6e0;
         inline constexpr uintptr_t LeftHipRangeMin = 0x6ec;
         inline constexpr uintptr_t LeftHipSize = 0x88c;
         inline constexpr uintptr_t LeftHipTposeAdjustment = 0x308;
         inline constexpr uintptr_t LeftKneeRangeMax = 0x6f8;
         inline constexpr uintptr_t LeftKneeRangeMin = 0x704;
         inline constexpr uintptr_t LeftKneeSize = 0x890;
         inline constexpr uintptr_t LeftKneeTposeAdjustment = 0x338;
         inline constexpr uintptr_t LeftShoulderRangeMax = 0x710;
         inline constexpr uintptr_t LeftShoulderRangeMin = 0x71c;
         inline constexpr uintptr_t LeftShoulderSize = 0x894;
         inline constexpr uintptr_t LeftShoulderTposeAdjustment = 0x368;
         inline constexpr uintptr_t LeftToeBaseRangeMax = 0x728;
         inline constexpr uintptr_t LeftToeBaseRangeMin = 0x734;
         inline constexpr uintptr_t LeftToeBaseSize = 0x898;
         inline constexpr uintptr_t LeftToeBaseTposeAdjustment = 0x398;
         inline constexpr uintptr_t LeftWristRangeMax = 0x740;
         inline constexpr uintptr_t LeftWristRangeMin = 0x74c;
         inline constexpr uintptr_t LeftWristSize = 0x89c;
         inline constexpr uintptr_t LeftWristTposeAdjustment = 0x3c8;
         inline constexpr uintptr_t NeckRangeMax = 0x758;
         inline constexpr uintptr_t NeckRangeMin = 0x764;
         inline constexpr uintptr_t NeckSize = 0x8a0;
         inline constexpr uintptr_t NeckTposeAdjustment = 0x3f8;
         inline constexpr uintptr_t OriginOffset = 0x428;
         inline constexpr uintptr_t RightAnkleRangeMax = 0x770;
         inline constexpr uintptr_t RightAnkleRangeMin = 0x77c;
         inline constexpr uintptr_t RightAnkleSize = 0x8a4;
         inline constexpr uintptr_t RightAnkleTposeAdjustment = 0x458;
         inline constexpr uintptr_t RightClavicleRangeMax = 0x788;
         inline constexpr uintptr_t RightClavicleRangeMin = 0x794;
         inline constexpr uintptr_t RightClavicleSize = 0x8a8;
         inline constexpr uintptr_t RightClavicleTposeAdjustment = 0x488;
         inline constexpr uintptr_t RightElbowRangeMax = 0x7a0;
         inline constexpr uintptr_t RightElbowRangeMin = 0x7ac;
         inline constexpr uintptr_t RightElbowSize = 0x8ac;
         inline constexpr uintptr_t RightElbowTposeAdjustment = 0x4b8;
         inline constexpr uintptr_t RightHipRangeMax = 0x7b8;
         inline constexpr uintptr_t RightHipRangeMin = 0x7c4;
         inline constexpr uintptr_t RightHipSize = 0x8b0;
         inline constexpr uintptr_t RightHipTposeAdjustment = 0x4e8;
         inline constexpr uintptr_t RightKneeRangeMax = 0x7d0;
         inline constexpr uintptr_t RightKneeRangeMin = 0x7dc;
         inline constexpr uintptr_t RightKneeSize = 0x8b4;
         inline constexpr uintptr_t RightKneeTposeAdjustment = 0x518;
         inline constexpr uintptr_t RightShoulderRangeMax = 0x7e8;
         inline constexpr uintptr_t RightShoulderRangeMin = 0x7f4;
         inline constexpr uintptr_t RightShoulderSize = 0x8b8;
         inline constexpr uintptr_t RightShoulderTposeAdjustment = 0x548;
         inline constexpr uintptr_t RightToeBaseRangeMax = 0x800;
         inline constexpr uintptr_t RightToeBaseRangeMin = 0x80c;
         inline constexpr uintptr_t RightToeBaseSize = 0x8bc;
         inline constexpr uintptr_t RightToeBaseTposeAdjustment = 0x578;
         inline constexpr uintptr_t RightWristRangeMax = 0x818;
         inline constexpr uintptr_t RightWristRangeMin = 0x824;
         inline constexpr uintptr_t RightWristSize = 0x8c0;
         inline constexpr uintptr_t RightWristTposeAdjustment = 0x5a8;
         inline constexpr uintptr_t RootRangeMax = 0x830;
         inline constexpr uintptr_t RootRangeMin = 0x83c;
         inline constexpr uintptr_t RootSize = 0x8c4;
         inline constexpr uintptr_t RootTposeAdjustment = 0x5d8;
         inline constexpr uintptr_t SpineRangeMax = 0x848;
         inline constexpr uintptr_t SpineRangeMin = 0x854;
         inline constexpr uintptr_t SpineSize = 0x8c8;
         inline constexpr uintptr_t SpineTposeAdjustment = 0x608;
         inline constexpr uintptr_t WaistRangeMax = 0x860;
         inline constexpr uintptr_t WaistRangeMin = 0x86c;
         inline constexpr uintptr_t WaistSize = 0x8cc;
         inline constexpr uintptr_t WaistTposeAdjustment = 0x638;
    }

    namespace IKControl {
         inline constexpr uintptr_t Enabled = 0x150;
         inline constexpr uintptr_t EndEffectorOffset = 0xe0;
         inline constexpr uintptr_t Offset = 0x110;
         inline constexpr uintptr_t Priority = 0x140;
         inline constexpr uintptr_t SmoothTime = 0x144;
         inline constexpr uintptr_t Weight = 0x14c;
    }

    namespace ImageButton {
         inline constexpr uintptr_t ContentImageSize = 0xa9c;
         inline constexpr uintptr_t Image = 0x9c0;
         inline constexpr uintptr_t ImageRectOffset = 0xaa4;
         inline constexpr uintptr_t ImageRectSize = 0xaac;
         inline constexpr uintptr_t ImageTransparency = 0xab4;
         inline constexpr uintptr_t SliceScale = 0xac0;
    }

    namespace ImageLabel {
         inline constexpr uintptr_t ContentImageSize = 0x7bc;
         inline constexpr uintptr_t Image = 0x740;
         inline constexpr uintptr_t ImageRectOffset = 0x7c4;
         inline constexpr uintptr_t ImageRectSize = 0x7cc;
         inline constexpr uintptr_t SliceCenter = 0x790;
         inline constexpr uintptr_t SliceScale = 0x7e0;
         inline constexpr uintptr_t TileSize = 0x7a0;
    }

    namespace IncrementalPatchBuilder {
         inline constexpr uintptr_t AddPathsToBundle = 0xb0;
         inline constexpr uintptr_t HighCompression = 0xb1;
         inline constexpr uintptr_t ZstdCompression = 0xb4;
    }

    namespace InputAction {
         inline constexpr uintptr_t Enabled = 0xf4;
    }

    namespace InputActionLabel {
         inline constexpr uintptr_t ImageTransparency = 0x6e8;
         inline constexpr uintptr_t TextSize = 0x6ec;
         inline constexpr uintptr_t TextTransparency = 0x6f0;
         inline constexpr uintptr_t TextWrapped = 0x6fc;
    }

    namespace InputBinding {
         inline constexpr uintptr_t ClampMagnitudeToOne = 0x168;
         inline constexpr uintptr_t Deadzone = 0x11c;
         inline constexpr uintptr_t PointerIndex = 0x140;
         inline constexpr uintptr_t PressedThreshold = 0x144;
         inline constexpr uintptr_t ResponseCurve = 0x150;
         inline constexpr uintptr_t Scale = 0x158;
         inline constexpr uintptr_t Vector2Scale = 0x124;
         inline constexpr uintptr_t Vector3Scale = 0x110;
    }

    namespace InputContext {
         inline constexpr uintptr_t Enabled = 0xa4;
         inline constexpr uintptr_t Priority = 0xa0;
         inline constexpr uintptr_t Sink = 0xa5;
    }

    namespace InsertService {
         inline constexpr uintptr_t AllowInsertFreeModels = 0xa8;
    }

    namespace Instance {
         inline constexpr uintptr_t ChildrenEnd = 0x8;
         inline constexpr uintptr_t ChildrenLayout = 0x1;
         inline constexpr uintptr_t ChildrenStart = 0x70;
         inline constexpr uintptr_t ClassBase = 0x1b0;
         inline constexpr uintptr_t ClassDescriptor = 0x18;
         inline constexpr uintptr_t ClassName = 0x8;
         inline constexpr uintptr_t Name = 0x0;
         inline constexpr uintptr_t NameContainer = 0x68;
         inline constexpr uintptr_t Parent = 0x60;
         inline constexpr uintptr_t This = 0x8;
    }

    namespace IntConstrainedValue {
         inline constexpr uintptr_t MaxValue = 0xa0;
         inline constexpr uintptr_t MinValue = 0xa8;
    }

    namespace InternalSyncItem {
         inline constexpr uintptr_t AutoSync = 0xc8;
         inline constexpr uintptr_t Enabled = 0xc9;
    }

    namespace IntValue {
         inline constexpr uintptr_t Value = 0xa0;
    }

    namespace JointInstance {
         inline constexpr uintptr_t Enabled = 0xa0;
         inline constexpr uintptr_t Part0 = 0x100;
         inline constexpr uintptr_t Part1 = 0x110;
    }

    namespace KeyframeSequence {
         inline constexpr uintptr_t AuthoredHipHeight = 0xc8;
    }

    namespace LayerCollector {
         inline constexpr uintptr_t Enabled = 0x260;
         inline constexpr uintptr_t TabKeyboardNavigation = 0x262;
    }

    namespace Light {
         inline constexpr uintptr_t Brightness = 0xb4;
         inline constexpr uintptr_t Enabled = 0xb8;
         inline constexpr uintptr_t Shadows = 0xb9;
    }

    namespace Lighting {
         inline constexpr uintptr_t Ambient = 0xb8;
         inline constexpr uintptr_t Brightness = 0x100;
         inline constexpr uintptr_t ClockTime = 0xb0;
         inline constexpr uintptr_t ColorShift_Bottom = 0xc4;
         inline constexpr uintptr_t ColorShift_Top = 0xd0;
         inline constexpr uintptr_t EnvironmentDiffuseScale = 0x104;
         inline constexpr uintptr_t EnvironmentSpecularScale = 0x108;
         inline constexpr uintptr_t ExposureCompensation = 0x10c;
         inline constexpr uintptr_t FogColor = 0xdc;
         inline constexpr uintptr_t FogEnd = 0x114;
         inline constexpr uintptr_t FogStart = 0x118;
         inline constexpr uintptr_t GeographicLatitude = 0x11c;
         inline constexpr uintptr_t GlobalShadows = 0x12c;
         inline constexpr uintptr_t GradientBottom = 0x178;
         inline constexpr uintptr_t GradientTop = 0x138;
         inline constexpr uintptr_t LightColor = 0x144;
         inline constexpr uintptr_t LightDirection = 0x150;
         inline constexpr uintptr_t MoonPosition = 0x16c;
         inline constexpr uintptr_t OutdoorAmbient = 0xe8;
         inline constexpr uintptr_t ShadowColor = 0xf4;
         inline constexpr uintptr_t ShadowSoftness = 0x124;
         inline constexpr uintptr_t Sky = 0x1b0;
         inline constexpr uintptr_t Source = 0x15c;
         inline constexpr uintptr_t SunPosition = 0x160;
    }

    namespace LinearVelocity {
         inline constexpr uintptr_t ReactionForceEnabled = 0x15c;
    }

    namespace LineForce {
         inline constexpr uintptr_t ApplyAtCenterOfMass = 0x160;
         inline constexpr uintptr_t MaxForce = 0x15c;
         inline constexpr uintptr_t ReactionForceEnabled = 0x162;
    }

    namespace LineHandleAdornment {
         inline constexpr uintptr_t Length = 0x190;
         inline constexpr uintptr_t Thickness = 0x194;
    }

    namespace LocalizationService {
         inline constexpr uintptr_t IsImageCaptureEnabled = 0x120;
         inline constexpr uintptr_t ShouldUseCloudTable = 0x121;
         inline constexpr uintptr_t ShouldUseImageLocalizationTable = 0x122;
    }

    namespace LocalizationTable {
         inline constexpr uintptr_t SourceLocaleId = 0xa0;
    }

    namespace LocalScript {
         inline constexpr uintptr_t ByteCode = 0x178;
         inline constexpr uintptr_t GUID = 0xb8;
         inline constexpr uintptr_t Hash = 0x188;
    }

    namespace LRUHolder {
         inline constexpr uintptr_t MemEnforcedLRUCache = 0x20;
    }

    namespace LRUNode {
         inline constexpr uintptr_t AssetID = 0x10;
         inline constexpr uintptr_t CachedItem = 0x38;
         inline constexpr uintptr_t Next = 0x0;
    }

    namespace LuaSourceContainer {
         inline constexpr uintptr_t CachedRemoteSource = 0xa0;
         inline constexpr uintptr_t ScriptGuid = 0xb8;
    }

    namespace MakeupDescription {
         inline constexpr uintptr_t AssetId = 0xa8;
         inline constexpr uintptr_t Order = 0xb4;
    }

    namespace MaterialColors {
         inline constexpr uintptr_t Asphalt = 0x30;
         inline constexpr uintptr_t Basalt = 0x27;
         inline constexpr uintptr_t Brick = 0xf;
         inline constexpr uintptr_t Cobblestone = 0x33;
         inline constexpr uintptr_t Concrete = 0xc;
         inline constexpr uintptr_t CrackedLava = 0x2d;
         inline constexpr uintptr_t Glacier = 0x1b;
         inline constexpr uintptr_t Grass = 0x6;
         inline constexpr uintptr_t Ground = 0x2a;
         inline constexpr uintptr_t Ice = 0x36;
         inline constexpr uintptr_t LeafyGrass = 0x39;
         inline constexpr uintptr_t Limestone = 0x3f;
         inline constexpr uintptr_t Mud = 0x24;
         inline constexpr uintptr_t Pavement = 0x42;
         inline constexpr uintptr_t Rock = 0x18;
         inline constexpr uintptr_t Salt = 0x3c;
         inline constexpr uintptr_t Sand = 0x12;
         inline constexpr uintptr_t Sandstone = 0x21;
         inline constexpr uintptr_t Slate = 0x9;
         inline constexpr uintptr_t Snow = 0x1e;
         inline constexpr uintptr_t WoodPlanks = 0x15;
    }

    namespace MaterialVariant {
         inline constexpr uintptr_t ColorMapContent = 0xa0;
         inline constexpr uintptr_t EmissiveMaskContent = 0xd0;
    }

    namespace MemEnforcedLRUCache {
         inline constexpr uintptr_t Head = 0x8;
    }

    namespace MeshContentProvider {
         inline constexpr uintptr_t LRUHolder = 0xc0;
    }

    namespace MeshPart {
         inline constexpr uintptr_t HasSkinnedMesh = 0x366;
         inline constexpr uintptr_t JointOffset = 0x350;
         inline constexpr uintptr_t MeshContent = 0x2e8;
         inline constexpr uintptr_t MeshId = 0x2f8;
         inline constexpr uintptr_t MeshSize = 0x210;
         inline constexpr uintptr_t Texture = 0x328;
         inline constexpr uintptr_t TextureContent = 0x318;
         inline constexpr uintptr_t VertexCount = 0x360;
    }

    namespace Message {
         inline constexpr uintptr_t Text = 0xa0;
    }

    namespace Misc {
         inline constexpr uintptr_t Adornee = 0xd8;
         inline constexpr uintptr_t AnimationId = 0xa8;
         inline constexpr uintptr_t StringLength = 0x10;
         inline constexpr uintptr_t Value = 0xa0;
    }

    namespace Model {
         inline constexpr uintptr_t ModelMeshCFrame = 0xe8;
         inline constexpr uintptr_t ModelMeshData = 0xd0;
         inline constexpr uintptr_t ModelMeshSize = 0x118;
         inline constexpr uintptr_t PrimaryPart = 0x240;
         inline constexpr uintptr_t Scale = 0x12c;
         inline constexpr uintptr_t SlimHash = 0xd8;
         inline constexpr uintptr_t WorldPivotData = 0xe0;
    }

    namespace ModuleScript {
         inline constexpr uintptr_t ByteCode = 0x120;
         inline constexpr uintptr_t GUID = 0xb8;
         inline constexpr uintptr_t Hash = 0x348;
         inline constexpr uintptr_t IsCoreScript = 0x0;
         inline constexpr uintptr_t LinkedSource = 0xf0;
         inline constexpr uintptr_t Source = 0x118;
    }

    namespace Motor6D {
         inline constexpr uintptr_t EnableSkinning = 0x198;
    }

    namespace MouseService {
         inline constexpr uintptr_t InputObject = 0xe8;
         inline constexpr uintptr_t InputObject2 = 0xf8;
         inline constexpr uintptr_t MousePosition = 0xbc;
         inline constexpr uintptr_t SensitivityPointer = 0x0;
    }

    namespace NoCollisionConstraint {
         inline constexpr uintptr_t Enabled = 0xc0;
    }

    namespace Noise {
         inline constexpr uintptr_t Seed = 0x9c;
    }

    namespace NumberPose {
         inline constexpr uintptr_t Value = 0xb8;
    }

    namespace NumberValue {
         inline constexpr uintptr_t Value = 0xa0;
    }

    namespace PackageLink {
         inline constexpr uintptr_t AutoUpdate = 0x124;
         inline constexpr uintptr_t CanAutoUpdate = 0x125;
         inline constexpr uintptr_t HasNewVersion = 0x126;
         inline constexpr uintptr_t ModifiedState = 0x120;
         inline constexpr uintptr_t PackageContentSerialize = 0xa0;
         inline constexpr uintptr_t PackageGuid = 0x110;
    }

    namespace Packages {
         inline constexpr uintptr_t IsDehydrated = 0xb0;
         inline constexpr uintptr_t OverridesRecalculated = 0xb1;
         inline constexpr uintptr_t ShellPackagesCount = 0xa8;
         inline constexpr uintptr_t SkippedInstancesCount = 0xac;
    }

    namespace Pages {
         inline constexpr uintptr_t IsFinished = 0x98;
    }

    namespace ParabolaAdornment {
         inline constexpr uintptr_t A = 0xe8;
         inline constexpr uintptr_t B = 0xec;
         inline constexpr uintptr_t Range = 0xf4;
         inline constexpr uintptr_t Thickness = 0xf8;
    }

    namespace ParticleEmitter {
         inline constexpr uintptr_t Acceleration = 0x1c8;
         inline constexpr uintptr_t Brightness = 0x204;
         inline constexpr uintptr_t Drag = 0x208;
         inline constexpr uintptr_t FlipbookBlendFrames = 0x251;
         inline constexpr uintptr_t FlipbookFramerate = 0x1d4;
         inline constexpr uintptr_t FlipbookSizeX = 0x218;
         inline constexpr uintptr_t FlipbookSizeY = 0x21c;
         inline constexpr uintptr_t FlipbookStartRandom = 0x252;
         inline constexpr uintptr_t Lifetime = 0x1dc;
         inline constexpr uintptr_t LightEmission = 0x220;
         inline constexpr uintptr_t LightInfluence = 0x224;
         inline constexpr uintptr_t LocalTransparencyModifier = 0x228;
         inline constexpr uintptr_t LockedToPart = 0x253;
         inline constexpr uintptr_t Rate = 0x230;
         inline constexpr uintptr_t RotSpeed = 0x1e4;
         inline constexpr uintptr_t Rotation = 0x1ec;
         inline constexpr uintptr_t ShapePartial = 0x23c;
         inline constexpr uintptr_t Size = 0xa0;
         inline constexpr uintptr_t Speed = 0x1f4;
         inline constexpr uintptr_t SpreadAngle = 0x1fc;
         inline constexpr uintptr_t Squash = 0xe0;
         inline constexpr uintptr_t Texture = 0x1a8;
         inline constexpr uintptr_t TimeScale = 0x244;
         inline constexpr uintptr_t VelocityInheritance = 0x248;
         inline constexpr uintptr_t WindAffectsDrag = 0x254;
         inline constexpr uintptr_t ZOffset = 0x24c;
    }

    namespace ParticleEmitter2D {
         inline constexpr uintptr_t Acceleration = 0x1e8;
         inline constexpr uintptr_t Blend = 0x228;
         inline constexpr uintptr_t Brightness = 0x22c;
         inline constexpr uintptr_t Drag = 0x230;
         inline constexpr uintptr_t EmissionAngle = 0x234;
         inline constexpr uintptr_t EmissionOffset = 0x1f0;
         inline constexpr uintptr_t EmissionSpread = 0x1f8;
         inline constexpr uintptr_t Enabled = 0x260;
         inline constexpr uintptr_t FlipbookBlendFrames = 0x261;
         inline constexpr uintptr_t FlipbookFramerate = 0x200;
         inline constexpr uintptr_t FlipbookSizeX = 0x244;
         inline constexpr uintptr_t FlipbookSizeY = 0x248;
         inline constexpr uintptr_t FlipbookStartRandom = 0x262;
         inline constexpr uintptr_t Lifetime = 0x208;
         inline constexpr uintptr_t LightInfluence = 0x24c;
         inline constexpr uintptr_t LockedToParent = 0x263;
         inline constexpr uintptr_t Rate = 0x250;
         inline constexpr uintptr_t RotSpeed = 0x210;
         inline constexpr uintptr_t Rotation = 0x218;
         inline constexpr uintptr_t Speed = 0x220;
         inline constexpr uintptr_t SpreadAngle = 0x254;
         inline constexpr uintptr_t TimeScale = 0x258;
         inline constexpr uintptr_t ZIndex = 0x25c;
    }

    namespace PartOperation {
         inline constexpr uintptr_t ChildData = 0x318;
         inline constexpr uintptr_t ChildData2 = 0x378;
         inline constexpr uintptr_t MeshData = 0x350;
         inline constexpr uintptr_t MeshData2 = 0x388;
         inline constexpr uintptr_t OffCentered = 0x3a4;
         inline constexpr uintptr_t SmoothingAngle = 0x39c;
         inline constexpr uintptr_t UsePartColor = 0x3a5;
    }

    namespace PartOperationAsset {
         inline constexpr uintptr_t MeshData = 0xb8;
    }

    namespace Path2D {
         inline constexpr uintptr_t SelectedControlPoint = 0xd4;
         inline constexpr uintptr_t Transparency = 0xdc;
    }

    namespace PathfindingLink {
         inline constexpr uintptr_t IsBidirectional = 0xe0;
         inline constexpr uintptr_t Label = 0xa0;
    }

    namespace PathfindingModifier {
         inline constexpr uintptr_t Label = 0xa0;
         inline constexpr uintptr_t PassThrough = 0xc0;
    }

    namespace PhysicsActivitySensor {
         inline constexpr uintptr_t Active = 0x238;
         inline constexpr uintptr_t Position = 0x220;
    }

    namespace Player {
         inline constexpr uintptr_t AccountAge = 0x364;
         inline constexpr uintptr_t AppearanceDidLoad = 0x3c8;
         inline constexpr uintptr_t AutoJumpEnabled = 0x3c9;
         inline constexpr uintptr_t BanningEnabled = 0x134;
         inline constexpr uintptr_t CameraFieldOfView = 0x36c;
         inline constexpr uintptr_t CameraFrustumRequested = 0x3ca;
         inline constexpr uintptr_t CameraMaxZoomDistance = 0x370;
         inline constexpr uintptr_t CameraMinZoomDistance = 0x374;
         inline constexpr uintptr_t CameraMode = 0x378;
         inline constexpr uintptr_t CameraViewportSize = 0x35c;
         inline constexpr uintptr_t CanLoadCharacterAppearance = 0x3cb;
         inline constexpr uintptr_t CharacterAppearanceId = 0x308;
         inline constexpr uintptr_t CharacterAutoLoads = 0x135;
         inline constexpr uintptr_t ChararacterRegionId = 0x350;
         inline constexpr uintptr_t CloudEditCameraCoordinateFrame = 0x320;
         inline constexpr uintptr_t CloudEditPlayerActive = 0x3cc;
         inline constexpr uintptr_t DataComplexityLimit = 0x380;
         inline constexpr uintptr_t DataReady = 0x3cd;
         inline constexpr uintptr_t DevEnableMouseLock = 0x3ce;
         inline constexpr uintptr_t DisplayName = 0x120;
         inline constexpr uintptr_t FollowUserId = 0x310;
         inline constexpr uintptr_t GameplayPaused = 0x3cf;
         inline constexpr uintptr_t HasRobloxSubscription = 0x3d0;
         inline constexpr uintptr_t HasVerifiedBadge = 0x3d1;
         inline constexpr uintptr_t HealthDisplayDistance = 0x3a0;
         inline constexpr uintptr_t InputLatency = 0x3a4;
         inline constexpr uintptr_t InternalCharacterAppearanceLoaded = 0x3d2;
         inline constexpr uintptr_t LocalPlayer = 0x118;
         inline constexpr uintptr_t LocaleId = 0x100;
         inline constexpr uintptr_t MaxPlayers = 0x128;
         inline constexpr uintptr_t MaxZoomDistance = 0x370;
         inline constexpr uintptr_t MaximumSimulationRadius = 0x3a8;
         inline constexpr uintptr_t MinZoomDistance = 0x374;
         inline constexpr uintptr_t ModelInstance = 0x2a0;
         inline constexpr uintptr_t Mouse = 0x11f0;
         inline constexpr uintptr_t NameDisplayDistance = 0x3b0;
         inline constexpr uintptr_t NeedRegionalFallback = 0x3d3;
         inline constexpr uintptr_t Neutral = 0x3d4;
         inline constexpr uintptr_t PreferredPlayers = 0x12c;
         inline constexpr uintptr_t RawJoinData = 0x1a0;
         inline constexpr uintptr_t RespawnTime = 0x130;
         inline constexpr uintptr_t SimulationRadius = 0x3b4;
         inline constexpr uintptr_t Team = 0x2e0;
         inline constexpr uintptr_t TeamColor = 0x3b8;
         inline constexpr uintptr_t Teleported = 0x3d5;
         inline constexpr uintptr_t TeleportedIn = 0x3d6;
         inline constexpr uintptr_t UseStrafingAnimations = 0x136;
         inline constexpr uintptr_t User = 0xa0;
         inline constexpr uintptr_t UserId = 0xb8;
         inline constexpr uintptr_t VREnabled = 0x3d8;
         inline constexpr uintptr_t VoiceChatVolume = 0x3c4;
    }

    namespace PlayerConfigurer {
         inline constexpr uintptr_t Pointer = 0x0;
    }

    namespace PlayerEmulatorService {
         inline constexpr uintptr_t TextElongationFactor = 0x110;
    }

    namespace PlayerListConfiguration {
         inline constexpr uintptr_t Open = 0xa0;
    }

    namespace PlayerMouse {
         inline constexpr uintptr_t Icon = 0xb0;
         inline constexpr uintptr_t Workspace = 0x138;
    }

    namespace PointLight {
         inline constexpr uintptr_t Range = 0xc0;
    }

    namespace Pose {
         inline constexpr uintptr_t CFrame = 0xb8;
    }

    namespace PoseBase {
         inline constexpr uintptr_t Weight = 0xa8;
    }

    namespace PostEffect {
         inline constexpr uintptr_t Enabled = 0x98;
    }

    namespace Primitive {
         inline constexpr uintptr_t AssemblyAngularVelocity = 0xec;
         inline constexpr uintptr_t AssemblyLinearVelocity = 0xe0;
         inline constexpr uintptr_t Flags = 0x1be;
         inline constexpr uintptr_t Material = 0x24e;
         inline constexpr uintptr_t Owner = 0x218;
         inline constexpr uintptr_t Position = 0xd4;
         inline constexpr uintptr_t Rotation = 0xb0;
         inline constexpr uintptr_t Size = 0x1c4;
         inline constexpr uintptr_t Validate = 0x6;
    }

    namespace PrimitiveFlags {
         inline constexpr uintptr_t Anchored = 0x2;
         inline constexpr uintptr_t CanCollide = 0x8;
         inline constexpr uintptr_t CanQuery = 0x20;
         inline constexpr uintptr_t CanTouch = 0x10;
    }

    namespace ProceduralModel {
         inline constexpr uintptr_t Dirty = 0x36c;
         inline constexpr uintptr_t Size = 0x360;
    }

    namespace ProximityPrompt {
         inline constexpr uintptr_t ActionText = 0x98;
         inline constexpr uintptr_t Enabled = 0x11e;
         inline constexpr uintptr_t GamepadKeyCode = 0x104;
         inline constexpr uintptr_t HoldDuration = 0x108;
         inline constexpr uintptr_t KeyCode = 0x10c;
         inline constexpr uintptr_t MaxActivationDistance = 0x110;
         inline constexpr uintptr_t MaxIndicatorDistance = 0x114;
         inline constexpr uintptr_t ObjectText = 0xb8;
         inline constexpr uintptr_t RequiresLineOfSight = 0x11f;
         inline constexpr uintptr_t UIOffset = 0xf8;
    }

    namespace ProximityPromptService {
         inline constexpr uintptr_t Enabled = 0xd0;
         inline constexpr uintptr_t MaxIndicatorsVisible = 0xc8;
         inline constexpr uintptr_t MaxPromptsVisible = 0xcc;
    }

    namespace PyramidHandleAdornment {
         inline constexpr uintptr_t DoubleSided = 0x1a0;
         inline constexpr uintptr_t Height = 0x190;
         inline constexpr uintptr_t Sides = 0x198;
         inline constexpr uintptr_t Size = 0x19c;
    }

    namespace QWidgetPluginGui {
         inline constexpr uintptr_t CloseButtonEnabled = 0x638;
    }

    namespace RealtimeMedia {
         inline constexpr uintptr_t AudioInputActive = 0x2a0;
         inline constexpr uintptr_t ForwardInput = 0x2a1;
         inline constexpr uintptr_t IsConnected = 0x2a2;
         inline constexpr uintptr_t IsConnectionWeak = 0x2a3;
    }

    namespace RenderingTest {
         inline constexpr uintptr_t Description = 0xa0;
         inline constexpr uintptr_t PerfTest = 0x124;
         inline constexpr uintptr_t RenderingTestFrameCount = 0x11c;
         inline constexpr uintptr_t ShouldSkip = 0x125;
         inline constexpr uintptr_t Ticket = 0xc0;
         inline constexpr uintptr_t Timeout = 0x120;
    }

    namespace RenderJob {
         inline constexpr uintptr_t FakeDataModel = 0x38;
         inline constexpr uintptr_t RealDataModel = 0x1f0;
         inline constexpr uintptr_t RenderView = 0x1e0;
    }

    namespace RenderView {
         inline constexpr uintptr_t DeviceD3D11 = 0x0;
         inline constexpr uintptr_t LightingValid = 0x0;
         inline constexpr uintptr_t SkyValid = 0x0;
         inline constexpr uintptr_t VisualEngine = 0x0;
    }

    namespace ReverbSoundEffect {
         inline constexpr uintptr_t Density = 0xe4;
         inline constexpr uintptr_t Diffusion = 0xe8;
         inline constexpr uintptr_t DryLevel = 0xec;
         inline constexpr uintptr_t WetLevel = 0xf0;
    }

    namespace RigidConstraint {
         inline constexpr uintptr_t EnableSkinning = 0x150;
    }

    namespace RocketPropulsion {
         inline constexpr uintptr_t Active = 0x2b8;
         inline constexpr uintptr_t CartoonFactor = 0x298;
         inline constexpr uintptr_t MaxSpeed = 0x29c;
         inline constexpr uintptr_t MaxThrust = 0x2a0;
         inline constexpr uintptr_t TargetRadius = 0x2a4;
         inline constexpr uintptr_t ThrustD = 0x2a8;
         inline constexpr uintptr_t ThrustP = 0x2ac;
         inline constexpr uintptr_t TurnD = 0x2b0;
         inline constexpr uintptr_t TurnP = 0x2b4;
    }

    namespace RodConstraint {
         inline constexpr uintptr_t LimitAngle0 = 0x15c;
         inline constexpr uintptr_t LimitAngle1 = 0x160;
         inline constexpr uintptr_t LimitsEnabled = 0x168;
         inline constexpr uintptr_t Thickness = 0x164;
    }

    namespace RolloutValidation {
         inline constexpr uintptr_t AdditionalFluffThree = 0x1d4;
         inline constexpr uintptr_t AdditionalFluffTwo = 0x390;
         inline constexpr uintptr_t CreationVersion = 0x1d8;
         inline constexpr uintptr_t FirstBinaryString = 0xb8;
         inline constexpr uintptr_t FirstSharedExpectedValue = 0xd8;
         inline constexpr uintptr_t FirstSharedString = 0x1b8;
         inline constexpr uintptr_t GenerationStrategy = 0x1dc;
         inline constexpr uintptr_t SecondBinaryExpectedValue = 0xf8;
         inline constexpr uintptr_t SecondBinaryString = 0x118;
         inline constexpr uintptr_t SecondSharedExpectedValue = 0x138;
         inline constexpr uintptr_t SecondSharedString = 0x1c0;
         inline constexpr uintptr_t ThirdBinaryExpectedValue = 0x158;
         inline constexpr uintptr_t ThirdBinaryString = 0x178;
         inline constexpr uintptr_t ThirdSharedExpectedValue = 0x198;
         inline constexpr uintptr_t ThirdSharedString = 0x1c8;
    }

    namespace RopeConstraint {
         inline constexpr uintptr_t Restitution = 0x15c;
         inline constexpr uintptr_t Thickness = 0x160;
         inline constexpr uintptr_t WinchForce = 0x164;
         inline constexpr uintptr_t WinchResponsiveness = 0x168;
         inline constexpr uintptr_t WinchTarget = 0x170;
    }

    namespace RTAnimationTracker {
         inline constexpr uintptr_t Active = 0x124;
         inline constexpr uintptr_t EnableFallbackAudioInput = 0x125;
    }

    namespace RunService {
         inline constexpr uintptr_t HeartbeatFPS = 0xc8;
         inline constexpr uintptr_t HeartbeatTask = 0x27c8;
    }

    namespace SafetyService {
         inline constexpr uintptr_t IsCaptureModeForReport = 0xa8;
    }

    namespace ScreenGui {
         inline constexpr uintptr_t DisplayOrder = 0x508;
         inline constexpr uintptr_t IgnoresTitleBarReservation = 0x515;
         inline constexpr uintptr_t OnTopOfCoreBlur = 0x516;
    }

    namespace ScreenshotHud {
         inline constexpr uintptr_t CloseButtonPosition = 0xd8;
         inline constexpr uintptr_t UsernameOverlayEnabled = 0xf0;
         inline constexpr uintptr_t Visible = 0xf1;
    }

    namespace Script {
         inline constexpr uintptr_t ByteCode = 0x178;
         inline constexpr uintptr_t GUID = 0xb8;
         inline constexpr uintptr_t Hash = 0x188;
         inline constexpr uintptr_t Source = 0x170;
    }

    namespace ScriptContext {
         inline constexpr uintptr_t RequireBypass = 0x0;
         inline constexpr uintptr_t ScriptsDisabled = 0xb0;
    }

    namespace ScrollingFrame {
         inline constexpr uintptr_t CanvasPosition = 0x73c;
         inline constexpr uintptr_t ScrollBarImageTransparency = 0x764;
         inline constexpr uintptr_t ScrollBarThickness = 0x768;
         inline constexpr uintptr_t ScrollRate = 0x76c;
         inline constexpr uintptr_t ScrollVelocity = 0x74c;
         inline constexpr uintptr_t SmoothScroll = 0x77d;
    }

    namespace Seat {
         inline constexpr uintptr_t Occupant = 0x200;
    }

    namespace Selection {
         inline constexpr uintptr_t ShowActiveInstanceHighlight = 0xb4;
    }

    namespace SelectionBox {
         inline constexpr uintptr_t LineThickness = 0xfc;
         inline constexpr uintptr_t StudioSelectionBox = 0x104;
         inline constexpr uintptr_t SurfaceTransparency = 0x100;
    }

    namespace SelectionPointLasso {
         inline constexpr uintptr_t Point = 0xf0;
    }

    namespace SelectionSphere {
         inline constexpr uintptr_t SurfaceTransparency = 0xfc;
    }

    namespace SelfViewConfiguration {
         inline constexpr uintptr_t Open = 0xa0;
    }

    namespace ServerScriptService {
         inline constexpr uintptr_t LoadStringEnabled = 0xb0;
    }

    namespace SkateboardController {
         inline constexpr uintptr_t Steer = 0x100;
         inline constexpr uintptr_t Throttle = 0x104;
    }

    namespace SkateboardPlatform {
         inline constexpr uintptr_t Steer = 0x214;
         inline constexpr uintptr_t Throttle = 0x218;
    }

    namespace Skin {
         inline constexpr uintptr_t SkinColor = 0xa8;
    }

    namespace Sky {
         inline constexpr uintptr_t CelestialBodiesShown = 0x238;
         inline constexpr uintptr_t MoonAngularSize = 0x22c;
         inline constexpr uintptr_t MoonTextureId = 0xb0;
         inline constexpr uintptr_t SkyboxBk = 0xe0;
         inline constexpr uintptr_t SkyboxDn = 0x110;
         inline constexpr uintptr_t SkyboxFt = 0x140;
         inline constexpr uintptr_t SkyboxLf = 0x170;
         inline constexpr uintptr_t SkyboxOrientation = 0x220;
         inline constexpr uintptr_t SkyboxRt = 0x1a0;
         inline constexpr uintptr_t SkyboxUp = 0x1d0;
         inline constexpr uintptr_t StarCount = 0x230;
         inline constexpr uintptr_t SunAngularSize = 0x224;
         inline constexpr uintptr_t SunTextureId = 0x200;
    }

    namespace SlidingBallConstraint {
         inline constexpr uintptr_t LinearResponsiveness = 0x15c;
         inline constexpr uintptr_t LowerLimit = 0x160;
         inline constexpr uintptr_t MotorMaxAcceleration = 0x164;
         inline constexpr uintptr_t MotorMaxForce = 0x168;
         inline constexpr uintptr_t ServoMaxForce = 0x170;
         inline constexpr uintptr_t Size = 0x174;
         inline constexpr uintptr_t Speed = 0x178;
         inline constexpr uintptr_t TargetPosition = 0x17c;
         inline constexpr uintptr_t UpperLimit = 0x180;
         inline constexpr uintptr_t Velocity = 0x184;
    }

    namespace SlimAnimationDataEntity {
         inline constexpr uintptr_t Handle = 0xf8;
         inline constexpr uintptr_t NumBones = 0xfc;
         inline constexpr uintptr_t RootIndex = 0x100;
    }

    namespace Smoke {
         inline constexpr uintptr_t Enabled = 0xc0;
         inline constexpr uintptr_t LocalTransparencyModifier = 0xac;
         inline constexpr uintptr_t RiseVelocity = 0xb8;
         inline constexpr uintptr_t Size = 0xbc;
         inline constexpr uintptr_t TimeScale = 0xb0;
    }

    namespace Sound {
         inline constexpr uintptr_t AcousticSimulationEnabled = 0x12c;
         inline constexpr uintptr_t IsPlaying = 0x130;
         inline constexpr uintptr_t LoopRegion = 0xe8;
         inline constexpr uintptr_t Looped = 0x12d;
         inline constexpr uintptr_t PlayOnRemove = 0x12e;
         inline constexpr uintptr_t PlaybackRegion = 0xf0;
         inline constexpr uintptr_t PlaybackRegionsEnabled = 0x12f;
         inline constexpr uintptr_t PlaybackSpeed = 0x10c;
         inline constexpr uintptr_t RollOffMaxDistance = 0x110;
         inline constexpr uintptr_t RollOffMinDistance = 0x114;
         inline constexpr uintptr_t SoundGroup = 0xd0;
         inline constexpr uintptr_t SoundId = 0xb0;
         inline constexpr uintptr_t TimePositionReplicator = 0xe0;
         inline constexpr uintptr_t Volume = 0x120;
    }

    namespace SoundEffect {
         inline constexpr uintptr_t Enabled = 0xa4;
         inline constexpr uintptr_t Priority = 0xa0;
    }

    namespace SoundService {
         inline constexpr uintptr_t AcousticSimulationEnabled = 0x160;
         inline constexpr uintptr_t DiffractionEnabled = 0x161;
         inline constexpr uintptr_t DistanceFactor = 0x14c;
         inline constexpr uintptr_t DopplerScale = 0x150;
         inline constexpr uintptr_t IsNewExpForAudioApiByDefault = 0x162;
         inline constexpr uintptr_t ListenerCFrame = 0x108;
         inline constexpr uintptr_t OcclusionEnabled = 0x163;
         inline constexpr uintptr_t RespectFilteringEnabled = 0x164;
         inline constexpr uintptr_t ReverbEnabled = 0x165;
         inline constexpr uintptr_t RolloffScale = 0x158;
    }

    namespace Sparkles {
         inline constexpr uintptr_t Enabled = 0xb4;
         inline constexpr uintptr_t LocalTransparencyModifier = 0xac;
         inline constexpr uintptr_t TimeScale = 0xb0;
    }

    namespace SpawnLocation {
         inline constexpr uintptr_t AllowTeamChangeOnTouch = 0x3d;
         inline constexpr uintptr_t Duration = 0x1d0;
         inline constexpr uintptr_t Enabled = 0x1d9;
         inline constexpr uintptr_t ForcefieldDuration = 0x1d0;
         inline constexpr uintptr_t Neutral = 0x1da;
         inline constexpr uintptr_t TeamColor = 0x1d4;
    }

    namespace SpecialMesh {
         inline constexpr uintptr_t MeshId = 0xe0;
         inline constexpr uintptr_t Scale = 0xac;
    }

    namespace SphereHandleAdornment {
         inline constexpr uintptr_t DoubleSided = 0x198;
         inline constexpr uintptr_t Radius = 0x190;
    }

    namespace SpotLight {
         inline constexpr uintptr_t Angle = 0xc0;
    }

    namespace SpringConstraint {
         inline constexpr uintptr_t Damping = 0x15c;
         inline constexpr uintptr_t FreeLength = 0x160;
         inline constexpr uintptr_t MaxForce = 0x164;
         inline constexpr uintptr_t MaxLength = 0x168;
         inline constexpr uintptr_t Radius = 0x170;
         inline constexpr uintptr_t Stiffness = 0x174;
         inline constexpr uintptr_t Thickness = 0x178;
    }

    namespace StarterGear {
         inline constexpr uintptr_t Loaded = 0x98;
    }

    namespace StarterGui {
         inline constexpr uintptr_t ShowDevelopmentGui = 0x17a;
    }

    namespace StarterPlayer {
         inline constexpr uintptr_t AllowCustomAnimations = 0x118;
         inline constexpr uintptr_t AutoJumpEnabled = 0x119;
         inline constexpr uintptr_t CameraMinZoomDistance = 0xc8;
         inline constexpr uintptr_t CharacterBreakJointsOnDeath = 0x11a;
         inline constexpr uintptr_t CharacterJumpHeight = 0xd0;
         inline constexpr uintptr_t CharacterJumpPower = 0xd4;
         inline constexpr uintptr_t CharacterMaxSlopeAngle = 0xd8;
         inline constexpr uintptr_t CharacterUseJumpPower = 0x11b;
         inline constexpr uintptr_t CharacterWalkSpeed = 0xdc;
         inline constexpr uintptr_t ClassicDeath = 0x11c;
         inline constexpr uintptr_t CreateDefaultPlayerModule = 0x11d;
         inline constexpr uintptr_t EnableMouseLockOption = 0x11e;
         inline constexpr uintptr_t HealthDisplayDistance = 0x100;
         inline constexpr uintptr_t LoadCharacterAppearance = 0x11f;
         inline constexpr uintptr_t NameDisplayDistance = 0x10c;
         inline constexpr uintptr_t PlayerModuleStatus = 0x110;
         inline constexpr uintptr_t UserEmotesEnabled = 0x120;
    }

    namespace StatsItem {
         inline constexpr uintptr_t Value = 0xe758;
    }

    namespace StudioData {
         inline constexpr uintptr_t EnableScriptCollabByDefaultOnLoad = 0xb0;
    }

    namespace StyleDerive {
         inline constexpr uintptr_t Priority = 0xb4;
    }

    namespace StyleQuery {
         inline constexpr uintptr_t IsActive = 0x98;
    }

    namespace StyleRule {
         inline constexpr uintptr_t Priority = 0x15c;
    }

    namespace SunRaysEffect {
         inline constexpr uintptr_t Enabled = 0x98;
         inline constexpr uintptr_t Intensity = 0xa0;
         inline constexpr uintptr_t Spread = 0xa4;
    }

    namespace SurfaceAppearance {
         inline constexpr uintptr_t AlphaMode = 0x1d8;
         inline constexpr uintptr_t Color = 0x1c0;
         inline constexpr uintptr_t ColorMap = 0xb0;
         inline constexpr uintptr_t ColorMapContent = 0xa0;
         inline constexpr uintptr_t EmissiveMaskContent = 0xe0;
         inline constexpr uintptr_t EmissiveStrength = 0x1dc;
         inline constexpr uintptr_t EmissiveTint = 0x1cc;
         inline constexpr uintptr_t MetalnessMap = 0x110;
         inline constexpr uintptr_t NormalMap = 0x140;
         inline constexpr uintptr_t RoughnessMap = 0x170;
    }

    namespace SurfaceGui {
         inline constexpr uintptr_t HorizontalCurvature = 0x60c;
    }

    namespace SurfaceGuiBase {
         inline constexpr uintptr_t Active = 0x51c;
    }

    namespace SurfaceLight {
         inline constexpr uintptr_t Angle = 0xc0;
    }

    namespace SwimController {
         inline constexpr uintptr_t PitchMaxTorque = 0xcc;
         inline constexpr uintptr_t PitchSpeedFactor = 0xd0;
         inline constexpr uintptr_t RollMaxTorque = 0xd4;
         inline constexpr uintptr_t RollSpeedFactor = 0xd8;
    }

    namespace TaskScheduler {
         inline constexpr uintptr_t JobEnd = 0xd0;
         inline constexpr uintptr_t JobName = 0x18;
         inline constexpr uintptr_t JobStart = 0xc8;
         inline constexpr uintptr_t MaxFPS = 0xb0;
         inline constexpr uintptr_t Pointer = 0x8f1e408;
    }

    namespace Team {
         inline constexpr uintptr_t AutoAssignable = 0xa4;
         inline constexpr uintptr_t AutoColorCharacters = 0xa5;
         inline constexpr uintptr_t BrickColor = 0xa0;
         inline constexpr uintptr_t ChildOrder = 0x98;
         inline constexpr uintptr_t Score = 0x9c;
         inline constexpr uintptr_t TeamColor = 0xa0;
    }

    namespace TeleportOptions {
         inline constexpr uintptr_t ShouldReserveServer = 0x118;
    }

    namespace TeleportService {
         inline constexpr uintptr_t CustomizedTeleportUI = 0xb0;
    }

    namespace Terrain {
         inline constexpr uintptr_t GrassLength = 0x1d8;
         inline constexpr uintptr_t MaterialColors = 0x4a0;
         inline constexpr uintptr_t SmoothVoxelsUpgraded = 0x1f2;
         inline constexpr uintptr_t WaterColor = 0x1c8;
         inline constexpr uintptr_t WaterReflectance = 0x1e0;
         inline constexpr uintptr_t WaterTransparency = 0x1e4;
         inline constexpr uintptr_t WaterWaveSize = 0x1e8;
         inline constexpr uintptr_t WaterWaveSpeed = 0x1ec;
    }

    namespace TerrainDetail {
         inline constexpr uintptr_t ColorMapContent = 0xa0;
         inline constexpr uintptr_t EmissiveMaskContent = 0xd0;
    }

    namespace TestFailure {
         inline constexpr uintptr_t Line = 0xb8;
    }

    namespace TestResult {
         inline constexpr uintptr_t FailedChecks = 0x100;
    }

    namespace TestRunResult {
         inline constexpr uintptr_t Failed = 0x80;
         inline constexpr uintptr_t Passed = 0x84;
         inline constexpr uintptr_t Skipped = 0x88;
         inline constexpr uintptr_t Total = 0x90;
    }

    namespace TestService {
         inline constexpr uintptr_t AutoRuns = 0xf8;
         inline constexpr uintptr_t ErrorCount = 0xe8;
         inline constexpr uintptr_t NumberOfPlayers = 0xec;
         inline constexpr uintptr_t WarnCount = 0xf4;
    }

    namespace TextBox {
         inline constexpr uintptr_t ContentText = 0xa28;
         inline constexpr uintptr_t LocalizationMatchIdentifier = 0xa48;
         inline constexpr uintptr_t LocalizationMatchedSourceText = 0xa68;
         inline constexpr uintptr_t OpenTypeFeatures = 0xaf0;
         inline constexpr uintptr_t TextScaled = 0xb5d;
         inline constexpr uintptr_t TextStrokeTransparency = 0xb3c;
         inline constexpr uintptr_t TextTransparency = 0xb40;
    }

    namespace TextButton {
         inline constexpr uintptr_t Confidential = 0xd9c;
         inline constexpr uintptr_t ContentText = 0xcb0;
         inline constexpr uintptr_t LocalizationMatchIdentifier = 0xcd0;
         inline constexpr uintptr_t LocalizationMatchedSourceText = 0xcf0;
         inline constexpr uintptr_t OpenTypeFeatures = 0xd58;
         inline constexpr uintptr_t TextStrokeTransparency = 0xd88;
         inline constexpr uintptr_t TextTransparency = 0xd8c;
    }

    namespace TextChannel {
         inline constexpr uintptr_t AddPlayersOnJoin = 0xa8;
    }

    namespace TextChannelWindow {
         inline constexpr uintptr_t IsRendering = 0x678;
         inline constexpr uintptr_t UseDefaultFont = 0x679;
    }

    namespace TextChatMessage {
         inline constexpr uintptr_t ForModeration = 0x2b4;
         inline constexpr uintptr_t IsHiddenMessage = 0x2b5;
         inline constexpr uintptr_t IsHistorical = 0x2b6;
         inline constexpr uintptr_t Verified = 0x2b7;
         inline constexpr uintptr_t WasRewritten = 0x2b8;
    }

    namespace TextChatService {
         inline constexpr uintptr_t CreateDefaultCommands = 0x1eb;
         inline constexpr uintptr_t CreateDefaultTextChannels = 0x1ec;
         inline constexpr uintptr_t HasSeenDeprecationDialog = 0x1ed;
         inline constexpr uintptr_t IsLegacyChatDisabled = 0x1ee;
    }

    namespace TextGenerator {
         inline constexpr uintptr_t Seed = 0xb8;
         inline constexpr uintptr_t Temperature = 0xbc;
         inline constexpr uintptr_t TopP = 0xc0;
    }

    namespace TextLabel {
         inline constexpr uintptr_t Confidential = 0xb1c;
         inline constexpr uintptr_t ContentText = 0xa30;
         inline constexpr uintptr_t LocalizationMatchIdentifier = 0xa50;
         inline constexpr uintptr_t LocalizationMatchedSourceText = 0xa70;
         inline constexpr uintptr_t OpenTypeFeatures = 0xad8;
         inline constexpr uintptr_t RichText = 0xb1d;
         inline constexpr uintptr_t Text = 0xa38;
         inline constexpr uintptr_t TextScaled = 0xb1f;
         inline constexpr uintptr_t TextStrokeTransparency = 0xb08;
         inline constexpr uintptr_t TextTransparency = 0xb0c;
    }

    namespace TextSource {
         inline constexpr uintptr_t CanSend = 0xe0;
         inline constexpr uintptr_t UserId = 0xd8;
    }

    namespace Textures {
         inline constexpr uintptr_t Decal_Texture = 0x1c8;
         inline constexpr uintptr_t Texture_Texture = 0x1c8;
    }

    namespace ThirdPartyUserService {
         inline constexpr uintptr_t HasActiveUser = 0xb0;
    }

    namespace Tool {
         inline constexpr uintptr_t CanBeDropped = 0x4a0;
         inline constexpr uintptr_t Enabled = 0x4a1;
         inline constexpr uintptr_t Grip = 0x494;
         inline constexpr uintptr_t ManualActivationOnly = 0x4a2;
         inline constexpr uintptr_t RequiresHandle = 0x4a3;
         inline constexpr uintptr_t TextureId = 0x348;
         inline constexpr uintptr_t ToolTip = 0x450;
         inline constexpr uintptr_t Tooltip = 0x450;
    }

    namespace TorsionSpringConstraint {
         inline constexpr uintptr_t Damping = 0x15c;
         inline constexpr uintptr_t MaxAngle = 0x160;
         inline constexpr uintptr_t MaxTorque = 0x164;
         inline constexpr uintptr_t Radius = 0x168;
         inline constexpr uintptr_t Stiffness = 0x170;
    }

    namespace Trail {
         inline constexpr uintptr_t Transparency = 0xa0;
         inline constexpr uintptr_t WidthScale = 0xe0;
    }

    namespace TremoloSoundEffect {
         inline constexpr uintptr_t Duty = 0xe4;
         inline constexpr uintptr_t Frequency = 0xe8;
    }

    namespace TriangleMeshPart {
         inline constexpr uintptr_t InertiaMigrated = 0x248;
         inline constexpr uintptr_t MeshSize = 0x210;
         inline constexpr uintptr_t PCDRequestId = 0x240;
         inline constexpr uintptr_t UnscaledVolume = 0x244;
    }

    namespace UIAspectRatioConstraint {
         inline constexpr uintptr_t AspectRatio = 0xb8;
    }

    namespace UIBlur {
         inline constexpr uintptr_t Enabled = 0xc4;
    }

    namespace UICorner {
         inline constexpr uintptr_t BottomLeftRadius = 0xb8;
         inline constexpr uintptr_t BottomRightRadius = 0xc0;
    }

    namespace UIDragDetector {
         inline constexpr uintptr_t DragAxis = 0x190;
         inline constexpr uintptr_t DragRotation = 0x1a0;
         inline constexpr uintptr_t DragUDim2 = 0x150;
         inline constexpr uintptr_t Enabled = 0x1c0;
         inline constexpr uintptr_t MaxDragAngle = 0x1ac;
         inline constexpr uintptr_t MaxDragTranslation = 0x160;
         inline constexpr uintptr_t MinDragAngle = 0x1b0;
         inline constexpr uintptr_t MinDragTranslation = 0x170;
         inline constexpr uintptr_t Selectable = 0x1c1;
         inline constexpr uintptr_t SelectionModeDragSpeed = 0x180;
         inline constexpr uintptr_t SelectionModeRotateSpeed = 0x1b8;
    }

    namespace UIFlexItem {
         inline constexpr uintptr_t GrowRatio = 0xb4;
         inline constexpr uintptr_t ShrinkRatio = 0xbc;
    }

    namespace UIGradient {
         inline constexpr uintptr_t Enabled = 0x140;
         inline constexpr uintptr_t Offset = 0x128;
         inline constexpr uintptr_t Rotation = 0x130;
    }

    namespace UIGridLayout {
         inline constexpr uintptr_t FillDirectionMaxCells = 0x178;
    }

    namespace UIListLayout {
         inline constexpr uintptr_t Wraps = 0x164;
    }

    namespace UIPadding {
         inline constexpr uintptr_t PaddingLeft = 0xb8;
         inline constexpr uintptr_t PaddingRight = 0xc0;
         inline constexpr uintptr_t PaddingTop = 0xc8;
    }

    namespace UIPageLayout {
         inline constexpr uintptr_t TouchInputEnabled = 0x168;
         inline constexpr uintptr_t TweenTime = 0x160;
    }

    namespace UIShadow {
         inline constexpr uintptr_t Inset = 0xf1;
         inline constexpr uintptr_t ShowBehindParent = 0xf2;
    }

    namespace UIStroke {
         inline constexpr uintptr_t BorderOffset = 0xc4;
         inline constexpr uintptr_t Thickness = 0xdc;
         inline constexpr uintptr_t Transparency = 0xe0;
         inline constexpr uintptr_t ZIndex = 0xe4;
    }

    namespace UITableLayout {
         inline constexpr uintptr_t FillEmptySpaceColumns = 0x15c;
    }

    namespace UnionOperation {
         inline constexpr uintptr_t AssetId = 0x2f8;
    }

    namespace UniversalConstraint {
         inline constexpr uintptr_t Radius = 0x15c;
         inline constexpr uintptr_t Restitution = 0x160;
    }

    namespace UserInputService {
         inline constexpr uintptr_t BottomBarSize = 0xb0;
         inline constexpr uintptr_t LegacyInputEventsEnabled = 0xd8;
         inline constexpr uintptr_t NavBarSize = 0xb8;
         inline constexpr uintptr_t RightBarSize = 0xc0;
         inline constexpr uintptr_t StatusBarSize = 0xc8;
         inline constexpr uintptr_t TouchScreenEnabled = 0xdb;
         inline constexpr uintptr_t WindowInputState = 0x288;
    }

    namespace VectorForce {
         inline constexpr uintptr_t ApplyAtCenterOfMass = 0x168;
    }

    namespace VehicleSeat {
         inline constexpr uintptr_t Disabled = 0x224;
         inline constexpr uintptr_t MaxSpeed = 0x210;
         inline constexpr uintptr_t SteerFloat = 0x214;
         inline constexpr uintptr_t ThrottleFloat = 0x218;
         inline constexpr uintptr_t Torque = 0x21c;
         inline constexpr uintptr_t TurnSpeed = 0x220;
    }

    namespace VersionControlService {
         inline constexpr uintptr_t ScriptCollabEnabled = 0x100;
    }

    namespace VideoDisplay {
         inline constexpr uintptr_t VideoTransparency = 0x75c;
    }

    namespace VideoFrame {
         inline constexpr uintptr_t Looped = 0x6c0;
         inline constexpr uintptr_t RollOffMaxDistance = 0x6b0;
         inline constexpr uintptr_t RollOffMinDistance = 0x6b4;
         inline constexpr uintptr_t VideoContent = 0x668;
         inline constexpr uintptr_t Volume = 0x6bc;
    }

    namespace VideoPlayer {
         inline constexpr uintptr_t AutoLoadInStudio = 0x169;
         inline constexpr uintptr_t AutoPlayInStudio = 0x16a;
         inline constexpr uintptr_t IsPlaying = 0x16b;
         inline constexpr uintptr_t PlaybackSpeed = 0x160;
         inline constexpr uintptr_t PlayingReplicating = 0x168;
         inline constexpr uintptr_t TimePosition = 0x150;
         inline constexpr uintptr_t VideoContent = 0x120;
         inline constexpr uintptr_t Volume = 0x164;
    }

    namespace ViewportFrame {
         inline constexpr uintptr_t CameraCFrame = 0x678;
         inline constexpr uintptr_t CameraFieldOfView = 0x6d8;
         inline constexpr uintptr_t ImageTransparency = 0x6dc;
         inline constexpr uintptr_t IsMirrored = 0x6e0;
    }

    namespace VisualEngine {
         inline constexpr uintptr_t Dimensions = 0xb10;
         inline constexpr uintptr_t FakeDataModel = 0xaf0;
         inline constexpr uintptr_t Pointer = 0x8a11808;
         inline constexpr uintptr_t RenderView = 0xc50;
         inline constexpr uintptr_t ViewMatrix = 0x1b0;
    }

    namespace VoiceChatService {
         inline constexpr uintptr_t EnableDefaultVoice = 0x1ec;
         inline constexpr uintptr_t UseNewAudioApi = 0x1ed;
         inline constexpr uintptr_t UseNewControlPaths = 0x1ee;
         inline constexpr uintptr_t UseNewJoinFlow = 0x1ef;
         inline constexpr uintptr_t UseStreamSwitching = 0x1f0;
         inline constexpr uintptr_t VoiceChatEnabledForPlaceOnRcc = 0x1f1;
         inline constexpr uintptr_t VoiceChatEnabledForUniverseOnRcc = 0x1f2;
    }

    namespace VRService {
         inline constexpr uintptr_t AvatarGestures = 0x11c;
         inline constexpr uintptr_t DidPointerHit = 0x11d;
         inline constexpr uintptr_t FadeOutViewOnCollision = 0x11e;
         inline constexpr uintptr_t PointerHitCFrame = 0xd0;
         inline constexpr uintptr_t QuestASWState = 0x11f;
         inline constexpr uintptr_t QuestDisplayRefreshRate = 0x114;
         inline constexpr uintptr_t VRDeviceAvailable = 0x120;
         inline constexpr uintptr_t VRDeviceName = 0xb0;
         inline constexpr uintptr_t VREnabled = 0x121;
    }

    namespace Weld {
         inline constexpr uintptr_t EnableSkinning = 0x170;
    }

    namespace WeldConstraint {
         inline constexpr uintptr_t Enabled = 0xc0;
         inline constexpr uintptr_t Part0 = 0xa0;
         inline constexpr uintptr_t Part1 = 0xb0;
    }

    namespace WindowInputState {
         inline constexpr uintptr_t CapsLock = 0x40;
         inline constexpr uintptr_t CurrentTextBox = 0x48;
    }

    namespace Wire {
         inline constexpr uintptr_t SourceName = 0xa0;
         inline constexpr uintptr_t TargetName = 0xc0;
    }

    namespace WireframeHandleAdornment {
         inline constexpr uintptr_t Scale = 0x190;
         inline constexpr uintptr_t Thickness = 0x19c;
    }

    namespace Workspace {
         inline constexpr uintptr_t CurrentCamera = 0x4a0;
         inline constexpr uintptr_t DistributedGameTime = 0x4c0;
         inline constexpr uintptr_t ReadOnlyGravity = 0x9c8;
         inline constexpr uintptr_t World = 0x3f8;
    }

    namespace World {
         inline constexpr uintptr_t AirProperties = 0x230;
         inline constexpr uintptr_t FallenPartsDestroyHeight = 0x20c;
         inline constexpr uintptr_t Gravity = 0x218;
         inline constexpr uintptr_t Primitives = 0x2a0;
         inline constexpr uintptr_t worldStepsPerSec = 0x868;
    }

    namespace WorldModel {
         inline constexpr uintptr_t UseWorkspaceCollisionGroups = 0x468;
    }

    namespace WorldRoot {
         inline constexpr uintptr_t GravityDirection = 0x370;
         inline constexpr uintptr_t SimulationRate = 0x38c;
         inline constexpr uintptr_t Wind = 0x390;
         inline constexpr uintptr_t WindDirection = 0x37c;
    }

    namespace WrapLayer {
         inline constexpr uintptr_t Color = 0x3b8;
         inline constexpr uintptr_t ReferenceMeshContent = 0x298;
         inline constexpr uintptr_t RotateScaleDeformationMap = 0x2c8;
    }

    namespace WrapTarget {
         inline constexpr uintptr_t Color = 0x298;
         inline constexpr uintptr_t Stiffness = 0x2a8;
    }

    namespace WrapTextureTransfer {
         inline constexpr uintptr_t UVMaxBound = 0xc8;
         inline constexpr uintptr_t UVMinBound = 0xd0;
    }
}
