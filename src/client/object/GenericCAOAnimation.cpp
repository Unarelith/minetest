/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "client/client.h"
#include "client/localplayer.h"
#include "client/object/GenericCAOAnimation.hpp"
#include "common/world/object_properties.h"
#include "util/serialize.h"

void GenericCAOAnimation::update(scene::IAnimatedMeshSceneNode *animated_meshnode)
{
	if (!animated_meshnode)
		return;

	if (animated_meshnode->getStartFrame() != m_animation_range.X ||
		animated_meshnode->getEndFrame() != m_animation_range.Y)
			animated_meshnode->setFrameLoop(m_animation_range.X, m_animation_range.Y);

	if (animated_meshnode->getAnimationSpeed() != m_animation_speed)
		animated_meshnode->setAnimationSpeed(m_animation_speed);

	animated_meshnode->setTransitionTime(m_animation_blend);

// Requires Irrlicht 1.8 or greater
#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR >= 8) || IRRLICHT_VERSION_MAJOR > 1
	if (animated_meshnode->getLoopMode() != m_animation_loop)
		animated_meshnode->setLoopMode(m_animation_loop);
#endif
}

void GenericCAOAnimation::updateSpeed(float speed, scene::IAnimatedMeshSceneNode *animated_meshnode)
{
	m_animation_speed = speed;

	if (!animated_meshnode)
		return;

	animated_meshnode->setAnimationSpeed(m_animation_speed);
}

void GenericCAOAnimation::updateData(std::istream &is, ClientEnvironment *env, bool is_local_player, scene::IAnimatedMeshSceneNode *animated_meshnode) {
	// TODO: change frames send as v2s32 value
	v2f range = readV2F1000(is);
	if (!is_local_player) {
		m_animation_range = v2s32((s32)range.X, (s32)range.Y);
		m_animation_speed = readF1000(is);
		m_animation_blend = readF1000(is);
		// these are sent inverted so we get true when the server sends nothing
		m_animation_loop = !readU8(is);
		update(animated_meshnode);
	} else {
		LocalPlayer *player = env->getLocalPlayer();
		if(player->last_animation == NO_ANIM)
		{
			m_animation_range = v2s32((s32)range.X, (s32)range.Y);
			m_animation_speed = readF1000(is);
			m_animation_blend = readF1000(is);
			// these are sent inverted so we get true when the server sends nothing
			m_animation_loop = !readU8(is);
		}
		// update animation only if local animations present
		// and received animation is unknown (except idle animation)
		bool is_known = false;
		for (int i = 1;i<4;i++)
		{
			if(m_animation_range.Y == player->local_animations[i].Y)
				is_known = true;
		}
		if(!is_known || (player->local_animations[1].Y + player->local_animations[2].Y < 1))
		{
			update(animated_meshnode);
		}
	}
}

void GenericCAOAnimation::updateFrame(float dtime) {
	m_anim_timer += dtime;
	if(m_anim_timer >= m_anim_framelength)
	{
		m_anim_timer -= m_anim_framelength;
		m_anim_frame++;
		if(m_anim_frame >= m_anim_num_frames)
			m_anim_frame = 0;
	}
}

void GenericCAOAnimation::animatePlayer(Client *client, LocalPlayer *player, scene::IAnimatedMeshSceneNode *animated_meshnode) {
	int old_anim = player->last_animation;
	float old_anim_speed = player->last_animation_speed;
	const PlayerControl &controls = player->getPlayerControl();

	bool walking = false;
	if (controls.up || controls.down || controls.left || controls.right ||
			controls.forw_move_joystick_axis != 0.f ||
			controls.sidew_move_joystick_axis != 0.f)
		walking = true;

	f32 new_speed = player->local_animation_speed;
	v2s32 new_anim = v2s32(0,0);
	bool allow_update = false;

	// increase speed if using fast or flying fast
	if((g_settings->getBool("fast_move") &&
				client->checkLocalPrivilege("fast")) &&
			(controls.aux1 ||
			 (!player->touching_ground &&
			  g_settings->getBool("free_move") &&
			  client->checkLocalPrivilege("fly"))))
		new_speed *= 1.5;
	// slowdown speed if sneeking
	if (controls.sneak && walking)
		new_speed /= 2;


	if (walking && (controls.LMB || controls.RMB)) {
		new_anim = player->local_animations[3];
		player->last_animation = WD_ANIM;
	} else if(walking) {
		new_anim = player->local_animations[1];
		player->last_animation = WALK_ANIM;
	} else if(controls.LMB || controls.RMB) {
		new_anim = player->local_animations[2];
		player->last_animation = DIG_ANIM;
	}

	// Apply animations if input detected and not attached
	// or set idle animation
	if ((new_anim.X + new_anim.Y) > 0 && !player->isAttached) {
		allow_update = true;
		m_animation_range = new_anim;
		m_animation_speed = new_speed;
		player->last_animation_speed = m_animation_speed;
	} else {
		player->last_animation = NO_ANIM;

		if (old_anim != NO_ANIM) {
			m_animation_range = player->local_animations[0];
			update(animated_meshnode);
		}
	}

	// Update local player animations
	if ((player->last_animation != old_anim ||
				m_animation_speed != old_anim_speed) &&
			player->last_animation != NO_ANIM && allow_update)
		update(animated_meshnode);
}

void GenericCAOAnimation::initTiles(const ObjectProperties &prop) {
	m_tx_size.X = 1.0 / prop.spritediv.X;
	m_tx_size.Y = 1.0 / prop.spritediv.Y;

	if(!m_initial_tx_basepos_set){
		m_initial_tx_basepos_set = true;
		m_tx_basepos = prop.initial_sprite_basepos;
	}
}

void GenericCAOAnimation::updateTiles(const v2s16 &pos, int frame_count, int frame_length, bool select_horiz_by_yawpitch) {
	m_tx_basepos = pos;
	m_anim_num_frames = frame_count;
	m_anim_framelength = frame_length;
	m_tx_select_horiz_by_yawpitch = select_horiz_by_yawpitch;
}

void GenericCAOAnimation::updateTexturePos(scene::IBillboardSceneNode *spritenode, const v3f &rotation)
{
	if (spritenode)
	{
		scene::ICameraSceneNode* camera = spritenode->getSceneManager()->getActiveCamera();
		if(!camera)
			return;

		v3f cam_to_entity = spritenode->getAbsolutePosition() - camera->getAbsolutePosition();
		cam_to_entity.normalize();

		int row = m_tx_basepos.Y;
		int col = m_tx_basepos.X;

		if (m_tx_select_horiz_by_yawpitch) {
			if (cam_to_entity.Y > 0.75)
				col += 5;
			else if (cam_to_entity.Y < -0.75)
				col += 4;
			else {
				float mob_dir = atan2(cam_to_entity.Z, cam_to_entity.X) / M_PI * 180.;
				float dir = mob_dir - rotation.Y;
				dir = wrapDegrees_180(dir);
				if (std::fabs(wrapDegrees_180(dir - 0)) <= 45.1f)
					col += 2;
				else if(std::fabs(wrapDegrees_180(dir - 90)) <= 45.1f)
					col += 3;
				else if(std::fabs(wrapDegrees_180(dir - 180)) <= 45.1f)
					col += 0;
				else if(std::fabs(wrapDegrees_180(dir + 90)) <= 45.1f)
					col += 1;
				else
					col += 4;
			}
		}

		// Animation goes downwards
		row += m_anim_frame;

		float txs = m_tx_size.X;
		float tys = m_tx_size.Y;
		setBillboardTextureMatrix(spritenode, txs, tys, col, row);
	}
}

void GenericCAOAnimation::setBillboardTextureMatrix(scene::IBillboardSceneNode *bill,
		float txs, float tys, int col, int row)
{
	video::SMaterial& material = bill->getMaterial(0);
	core::matrix4& matrix = material.getTextureMatrix(0);
	matrix.setTextureTranslate(txs*col, tys*row);
	matrix.setTextureScale(txs, tys);
}


