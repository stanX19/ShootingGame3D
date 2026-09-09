#pragma once

#include "includes.hpp"
#include "model_manager.hpp"

struct RenderBody
{
	t_model_id modelID;
	Color color;
	Vector3 scale;
	Vector3 translation;
	Quaternion rotation;

	RenderBody(t_model_id id, float _scale)
		: modelID(id), color(WHITE), translation({0, 0, 0}), rotation({0, 0, 0, 0})
	{
		scale = Vector3{_scale, _scale, _scale};
	}

	RenderBody(t_model_id id, Color color, float _scale, Vector3 _translation = {0.0f, 0.0f, 0.0f}, Quaternion _rotation = {0.0f, 0.0f, 0.0f, 1.0f})
		: modelID(id), color(color), translation(_translation), rotation(_rotation)
	{
		scale = Vector3{_scale, _scale, _scale};
	}

	RenderBody(
		t_model_id id,
		Color _color = WHITE,
		Vector3 _scale = {1.0f, 1.0f, 1.0f},
		Vector3 _translation = {0.0f, 0.0f, 0.0f},
		Quaternion _rotation = {0.0f, 0.0f, 0.0f, 1.0f}
	)
		: modelID(id), color(_color), scale(_scale), translation(_translation), rotation(_rotation)
	{}
};

struct ModelStrech {
	float scale;
};

struct RadiusExpand {
	float speed;
};

namespace tag {
	struct LightSource {};
	struct Shaded {};
	struct SkyBox {};
	struct RotationSyncModel {};
	struct AimDirectionSyncModel {};
	struct VelocitySyncModelRot {};
	struct VelocitySyncRot {};
}
