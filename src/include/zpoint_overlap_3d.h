/*
* Copyright (C) 2019 YaweiZhang <yawei.zhang@foxmail.com>.
* All rights reserved
* This file is part of the zbase, used MIT License.
*/


#pragma once
#ifndef  ZPOINT_OVERLAP_3D_H
#define ZPOINT_OVERLAP_3D_H

#include "zpoint.h"
#include <utility>
#include <tuple>

class zpoint_hit
{
public:
	zpoint_hit(zpoint hit_pos, zpoint hit_norm) :hit_pos_(hit_pos), hit_norm_(hit_norm) {}
	zpoint_hit(const zpoint_hit&) = default;
	zpoint_hit(){};
	zpoint_hit& operator =(const zpoint_hit& v3) = default;

	zpoint hit_pos_; //collistion pos.  aways use target shape    
	zpoint hit_norm_; // face to target 
};


class zpoint_capsule
{
public:
	zpoint a_;
	zpoint b_;
	float r_;
public:

public:
	zpoint_capsule(const zpoint& a, const zpoint& b, float r) : a_(a), b_(b), r_(r) {}
	zpoint_capsule(const zpoint_capsule&) = default;
	zpoint_capsule() : zpoint_capsule(zpoint(), zpoint(), 0.0f) {};
	zpoint_capsule& operator =(const zpoint_capsule& v3) = default;

	void reset() { a_.reset(); b_.reset(); r_ = 0.0f; }


};



class zpoint_collision
{
public:
	bool sphere_sphere_bool(const zpoint& spos, float sr, const zpoint tpos, float tr)
	{
		return (spos - tpos).square_distance() < (sr + tr) * (sr + tr);
	}

	std::pair<bool, zpoint_hit> sphere_sphere_pos(const zpoint& spos, float sr, const zpoint& tpos, float tr)
	{
		std::pair<bool, zpoint_hit> result;
		if (sphere_sphere_bool(spos, sr, tpos, tr))
		{
			result.first = true;
			result.second.hit_norm_ = (spos - tpos).const_normalize();
			result.second.hit_pos_ = tpos + result.second.hit_norm_ * tr;
			return result;
		}
		result.first = false;
		return result;
	}

	std::pair<bool, zpoint_hit> sphere_capsule(const zpoint& spos, float sr, const zpoint_capsule& tcapsule)
	{
		std::pair<bool, zpoint_hit> result;

		zpoint u = tcapsule.b_ - tcapsule.a_;
		zpoint v = spos - tcapsule.a_;
		float r = sr + tcapsule.r_;

		//degeneration:  capsule as a sphere  
		if (u.square_length() <= zpoint::kFloatPrecision)
		{
			result = sphere_sphere_pos(spos, sr, tcapsule.a_, tcapsule.r_);
			return result;
		}

		//collistion or overlap on pos tcapsule.a_ 
		if (v.square_length() <= (std::max)(r * r, zpoint::kFloatPrecision))
		{
			result.first = true;
			result.second.hit_norm_ = v.const_normalize();
			result.second.hit_pos_ = tcapsule.a_ + result.second.hit_norm_ * tcapsule.r_;
			return result;
		}


		//float cos_w = u.dot(v) / (u.distance() * v.distance());
		
		//float projection_len = u.dot(v) * v.distance()/ (u.distance() * v.distance()) 
		
		float dot_uv = u.dot(v);

		//negative dir.  use tcapsule.a_ check distance;   
		if (dot_uv <= 0.0f)
		{
			result = sphere_sphere_pos(spos, sr, tcapsule.a_, tcapsule.r_);
			return result;
		}

		float u_len = u.length();
		float projection_len = u.dot(v) / u_len;
		
		//find the shortest pos 
		projection_len = (std::min)(projection_len, u_len);

		
		result = sphere_sphere_pos(spos, sr, tcapsule.a_ + u.const_normalize() * projection_len, tcapsule.r_);
		return result;
	}


	std::pair<bool, zpoint_hit> capsule_sphere(const zpoint_capsule& scapsule, const zpoint& tpos, float tr)
	{
		std::pair<bool, zpoint_hit> result = sphere_capsule(tpos, tr, scapsule);
		if (result.first)
		{
			result.second.hit_norm_ *= -1;
			result.second.hit_pos_ = tpos + result.second.hit_norm_ * tr;
		}
		return result;
	}

	std::pair<bool, zpoint_hit> capsule_capsule(const zpoint_capsule& scapsule, const zpoint_capsule& tcapsule)
	{
		std::pair<bool, zpoint_hit> result;

		zpoint u = scapsule.b_ - scapsule.a_;
		zpoint v = tcapsule.b_ - tcapsule.a_;

		//degeneration:  scapsule as a sphere  
		if (u.square_length() < zpoint::kFloatPrecision)
		{
			result = sphere_capsule(scapsule.a_, scapsule.r_, tcapsule);
			return result;
		}

		//degeneration:  tcapsule as a sphere 
		if (v.square_length() < zpoint::kFloatPrecision)
		{
			result = capsule_sphere(scapsule, tcapsule.a_, tcapsule.r_);
			return result;
		}

		zpoint cross_uv = u.cross(v);

		//parallel or collinear 
		if (cross_uv.square_length() <= zpoint::kFloatPrecision)
		{
			float projection_len = u.dot(tcapsule.a_ - scapsule.a_) / u.length();
			zpoint virtual_pos = scapsule.a_ + u.length() * projection_len;

			//
			if ((virtual_pos - tcapsule.a_).length() <= (std::max)(zpoint::kFloatPrecision, scapsule.r_ + tcapsule.r_))
			{
				//collinear
				// 
				// virtual_pos = scapsule.a_ + u.normalize() * projection_len;
				if (projection_len >= 0 && projection_len <= u.length())
				{
					result.first = true;
					result.second.hit_norm_ = (virtual_pos - tcapsule.a_).const_normalize();
					result.second.hit_pos_ = tcapsule.a_ + result.second.hit_norm_ * tcapsule.r_;
					return result;
				}

				projection_len = u.dot(tcapsule.b_ - scapsule.a_) / u.length();
				virtual_pos = scapsule.a_ + u.normalize() * projection_len;
				if (projection_len >= 0 && projection_len <= u.length())
				{
					result.first = true;
					result.second.hit_norm_ = (virtual_pos - tcapsule.b_).const_normalize();
					result.second.hit_pos_ = tcapsule.b_ + result.second.hit_norm_ * tcapsule.r_;
					return result;
				}


				projection_len = v.dot(scapsule.a_ - tcapsule.a_) / v.length();
				virtual_pos = tcapsule.a_ + v.normalize() * projection_len;
				if (projection_len >= 0 && projection_len <= v.length())
				{
					result.first = true;
					result.second.hit_norm_ = (scapsule.a_ - virtual_pos).const_normalize();
					result.second.hit_pos_ = virtual_pos;
					return result;
				}

				projection_len = v.dot(scapsule.b_ - tcapsule.a_) / v.length();
				virtual_pos = tcapsule.a_ + v.normalize() * projection_len;
				if (projection_len >= 0 && projection_len <= v.length())
				{
					result.first = true;
					result.second.hit_norm_ = (scapsule.b_ - virtual_pos).const_normalize();
					result.second.hit_pos_ = virtual_pos;
					return result;
				}


				//if no hit pos and hit dir.  this code can use MINSS optimize 
				//(std::min)
				std::tuple<zpoint, zpoint, float> short_dist[4] =
				{
					{tcapsule.a_, (scapsule.a_ - tcapsule.a_), (scapsule.a_ - tcapsule.a_).square_length()},
					{tcapsule.a_, (scapsule.b_ - tcapsule.a_), (scapsule.b_ - tcapsule.a_).square_length()},
					{tcapsule.b_, (scapsule.a_ - tcapsule.b_), (scapsule.a_ - tcapsule.b_).square_length()},
					{tcapsule.b_, (scapsule.b_ - tcapsule.b_), (scapsule.b_ - tcapsule.b_).square_length()}
				};

				s32 idx = 0;
				float dist = std::get<2>(short_dist[0]);

				for (s32 i = 1; i < 4; i++)
				{
					if (dist > std::get<2>(short_dist[i]))
					{
						idx = i;
						dist = std::get<2>(short_dist[i]);
					}
				}

				if (std::sqrt(dist) <= tcapsule.r_ + scapsule.r_)
				{
					result.first = true;
					result.second.hit_norm_ = std::get<1>(short_dist[idx]).const_normalize();
					result.second.hit_pos_ = std::get<0>(short_dist[idx]) + result.second.hit_norm_ * tcapsule.r_;
					return result;
				}
			}
			result.first = false;
			return result;
		}


		//global shortest dist
		float dist = (scapsule.a_ - tcapsule.a_).dot(cross_uv) / cross_uv.distance();

		if (dist > scapsule.r_ + tcapsule.r_)
		{
			result.first = false;
			return result;
		}




		//todo 
		
		return result;
	}



};


#endif