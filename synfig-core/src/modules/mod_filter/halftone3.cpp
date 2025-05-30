/* === S Y N F I G ========================================================= */
/*!	\file halftone3.cpp
**	\brief Implementation of the "Halftone 3" layer
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2007-2008 Chris Moore
**	Copyright (c) 2012-2013 Carlos López
**
**	This file is part of Synfig.
**
**	Synfig is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 2 of the License, or
**	(at your option) any later version.
**
**	Synfig is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with Synfig.  If not, see <https://www.gnu.org/licenses/>.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "halftone3.h"
#include "halftone.h"

#include <synfig/localization.h>

#include <synfig/string.h>
#include <synfig/context.h>
#include <synfig/paramdesc.h>
#include <synfig/rendering/software/task/taskpaintpixelsw.h>
#include <synfig/value.h>

#endif

/* === M A C R O S ========================================================= */

using namespace synfig;

/* === G L O B A L S ======================================================= */

SYNFIG_LAYER_INIT(Halftone3);
SYNFIG_LAYER_SET_NAME(Halftone3,"halftone3");
SYNFIG_LAYER_SET_LOCAL_NAME(Halftone3,N_("Halftone 3"));
SYNFIG_LAYER_SET_CATEGORY(Halftone3,N_("Filters"));
SYNFIG_LAYER_SET_VERSION(Halftone3,"0.1");

/* === P R O C E D U R E S ================================================= */

class TaskHalfTone3: public rendering::TaskPixelProcessorBase, public rendering::TaskInterfaceTransformationGetAndPass
{
public:
	typedef etl::handle<TaskHalfTone3> Handle;
	SYNFIG_EXPORT static Token token;
	Token::Handle get_token() const override { return token.handle(); }

	Halftone tone[3];
	Color color[3];
	bool subtractive = false;
	float inverse_matrix[3][3];

	rendering::Holder<rendering::TransformationAffine> transformation;

	rendering::Transformation::Handle get_transformation() const override
	{
		return transformation.handle();
	}
};


class TaskHalfTone3SW: public TaskHalfTone3, public rendering::TaskFilterPixelSW
{
public:
	typedef etl::handle<TaskHalfTone3> Handle;
	SYNFIG_EXPORT static Token token;
	Token::Handle get_token() const override { return token.handle(); }

	void pre_run(const Matrix3& /*matrix*/) const override
	{
		supersample = 1/std::fabs(get_pixels_per_unit()[0]*(tone[0].param_size.get(Vector())).mag());
		inverted_transformation = transformation->create_inverted();
	}

	Color get_color(const Vector& p, const Color& in_color) const override
	{
		Color halfcolor;

		float chan[3];

		const Vector point = inverted_transformation->transform(p);

		if(subtractive)
		{
			chan[0]=inverse_matrix[0][0]*(1.0f-in_color.get_r())+inverse_matrix[0][1]*(1.0f-in_color.get_g())+inverse_matrix[0][2]*(1.0f-in_color.get_b());
			chan[1]=inverse_matrix[1][0]*(1.0f-in_color.get_r())+inverse_matrix[1][1]*(1.0f-in_color.get_g())+inverse_matrix[1][2]*(1.0f-in_color.get_b());
			chan[2]=inverse_matrix[2][0]*(1.0f-in_color.get_r())+inverse_matrix[2][1]*(1.0f-in_color.get_g())+inverse_matrix[2][2]*(1.0f-in_color.get_b());

			halfcolor=Color::white();
			halfcolor-=(~color[0])*tone[0](point,chan[0],supersample);
			halfcolor-=(~color[1])*tone[1](point,chan[1],supersample);
			halfcolor-=(~color[2])*tone[2](point,chan[2],supersample);

			halfcolor.set_a(in_color.get_a());
		}
		else
		{
			chan[0]=inverse_matrix[0][0]*in_color.get_r()+inverse_matrix[0][1]*in_color.get_g()+inverse_matrix[0][2]*in_color.get_b();
			chan[1]=inverse_matrix[1][0]*in_color.get_r()+inverse_matrix[1][1]*in_color.get_g()+inverse_matrix[1][2]*in_color.get_b();
			chan[2]=inverse_matrix[2][0]*in_color.get_r()+inverse_matrix[2][1]*in_color.get_g()+inverse_matrix[2][2]*in_color.get_b();

			halfcolor=Color::black();
			halfcolor+=color[0]*tone[0](point,chan[0],supersample);
			halfcolor+=color[1]*tone[1](point,chan[1],supersample);
			halfcolor+=color[2]*tone[2](point,chan[2],supersample);

			halfcolor.set_a(in_color.get_a());
		}

		return halfcolor;
	}

	bool run(RunParams&) const override
	{
		return run_task();
	}

protected:
	mutable float supersample = 1.0f;
	mutable rendering::Transformation::Handle inverted_transformation;
};

SYNFIG_EXPORT rendering::Task::Token TaskHalfTone3::token(
	DescAbstract<TaskHalfTone3>("HalfTone3") );
SYNFIG_EXPORT rendering::Task::Token TaskHalfTone3SW::token(
	DescReal<TaskHalfTone3SW, TaskHalfTone3>("HalfTone3SW") );

/* === M E T H O D S ======================================================= */

Halftone3::Halftone3()
	: Layer_CompositeFork(1.0, Color::BLEND_STRAIGHT),
	  param_size(ValueBase(Vector(0.25, 0.25))),
	  param_type(ValueBase(int(TYPE_SYMMETRIC)))
{
	for(int i=0;i<3;i++)
	{
		tone[i].param_size=param_size;
		tone[i].param_type=param_type;
		tone[i].param_origin=ValueBase(synfig::Point(0,0));
		tone[i].param_angle=ValueBase(Angle::deg(30.0)*(float)i);
	}

	param_subtractive=ValueBase(true);

	if(param_subtractive.get(bool()))
	{
		param_color[0].set(Color::cyan());
		param_color[1].set(Color::magenta());
		param_color[2].set(Color::yellow());
	}
	else
	{
		param_color[0].set(Color::red());
		param_color[1].set(Color::green());
		param_color[2].set(Color::blue());
	}

	set_blend_method(Color::BLEND_STRAIGHT);

	for(int i=0;i<3;i++)
		for(int j=0;j<3;j++)
			inverse_matrix[i][j]=(j==i)?1.0f:0.0f;

	sync();
	
	SET_INTERPOLATION_DEFAULTS();
	SET_STATIC_DEFAULTS();
}

void
Halftone3::sync()
{
	bool subtractive=param_subtractive.get(bool());
	Color color[3];
	for(int i=0;i<3;i++)
		color[i]=param_color[i].get(Color());

	// Is this needed? set_param does the same!
	for(int i=0;i<3;i++)
	{
		tone[i].param_size=param_size;
		tone[i].param_type=param_type;
	}

#define matrix inverse_matrix
	//float matrix[3][3];
	if(subtractive)
	{
		for(int i=0;i<3;i++)
		{
			matrix[i][0]=1.0f-(color[i].get_r());
			matrix[i][1]=1.0f-(color[i].get_g());
			matrix[i][2]=1.0f-(color[i].get_b());
			float mult=sqrt(matrix[i][0]*matrix[i][0]+matrix[i][1]*matrix[i][1]+matrix[i][2]*matrix[i][2]);
			if(mult)
			{
				matrix[i][0]/=mult;
				matrix[i][1]/=mult;
				matrix[i][2]/=mult;
				matrix[i][0]/=mult;
				matrix[i][1]/=mult;
				matrix[i][2]/=mult;
			}
		}
	}
	else
	{
		for(int i=0;i<3;i++)
		{
			matrix[i][0]=color[i].get_r();
			matrix[i][1]=color[i].get_g();
			matrix[i][2]=color[i].get_b();
			float mult=sqrt(matrix[i][0]*matrix[i][0]+matrix[i][1]*matrix[i][1]+matrix[i][2]*matrix[i][2]);
			if(mult)
			{
				matrix[i][0]/=mult;
				matrix[i][1]/=mult;
				matrix[i][2]/=mult;
				matrix[i][0]/=mult;
				matrix[i][1]/=mult;
				matrix[i][2]/=mult;
			}
		}
	}
#undef matrix



#if 0
	// Insert guass-jordan elimination code here
	int k=0,i=0,j=0,z_size=3;
#define A inverse_matrix

	for (k=0;k<z_size;k++)
  // the pivot element
    { A[k][k]= -1/A[k][k];

  //the pivot column
     for (i=0;i<z_size;i++)
         if (i!=k) A[i][k]*=A[k][k];

 //elements not in a pivot row or column
     for (i=0;i<z_size;i++)
        if (i!=k)
            for (j=0;j<z_size;j++)
                      if (j!=k)
                          A[i][j]+=A[i][k]*A[k][j];

 //elements in a pivot row
    for (i=0;i<z_size;i++)
       if (i!=k)
            A[k][i]*=A[k][k];
   }

 //change sign
   for (i=0;i<z_size;i++)        /*reverse sign*/
     for (j=0;j<z_size;j++)
        A[i][j]=-A[i][j];
#undef A
#endif
}

inline Color
Halftone3::color_func(const Point &point, float supersample,const Color& in_color)const
{
	bool subtractive=param_subtractive.get(bool());
	Color color[3];
	for(int i=0;i<3;i++)
		color[i]=param_color[i].get(Color());

	Color halfcolor;

	float chan[3];


	if(subtractive)
	{
		chan[0]=inverse_matrix[0][0]*(1.0f-in_color.get_r())+inverse_matrix[0][1]*(1.0f-in_color.get_g())+inverse_matrix[0][2]*(1.0f-in_color.get_b());
		chan[1]=inverse_matrix[1][0]*(1.0f-in_color.get_r())+inverse_matrix[1][1]*(1.0f-in_color.get_g())+inverse_matrix[1][2]*(1.0f-in_color.get_b());
		chan[2]=inverse_matrix[2][0]*(1.0f-in_color.get_r())+inverse_matrix[2][1]*(1.0f-in_color.get_g())+inverse_matrix[2][2]*(1.0f-in_color.get_b());

		halfcolor=Color::white();
		halfcolor-=(~color[0])*tone[0](point,chan[0],supersample);
		halfcolor-=(~color[1])*tone[1](point,chan[1],supersample);
		halfcolor-=(~color[2])*tone[2](point,chan[2],supersample);

		halfcolor.set_a(in_color.get_a());
	}
	else
	{
		chan[0]=inverse_matrix[0][0]*in_color.get_r()+inverse_matrix[0][1]*in_color.get_g()+inverse_matrix[0][2]*in_color.get_b();
		chan[1]=inverse_matrix[1][0]*in_color.get_r()+inverse_matrix[1][1]*in_color.get_g()+inverse_matrix[1][2]*in_color.get_b();
		chan[2]=inverse_matrix[2][0]*in_color.get_r()+inverse_matrix[2][1]*in_color.get_g()+inverse_matrix[2][2]*in_color.get_b();

		halfcolor=Color::black();
		halfcolor+=color[0]*tone[0](point,chan[0],supersample);
		halfcolor+=color[1]*tone[1](point,chan[1],supersample);
		halfcolor+=color[2]*tone[2](point,chan[2],supersample);

		halfcolor.set_a(in_color.get_a());
	}

	return halfcolor;
}

synfig::Layer::Handle
Halftone3::hit_check(synfig::Context /*context*/, const synfig::Point &/*point*/)const
{
	return const_cast<Halftone3*>(this);
}

bool
Halftone3::set_param(const String & param, const ValueBase &value)
{
	IMPORT_VALUE_PLUS(param_size,
		{
			for(int i=0;i<3;i++)
				tone[i].param_size=param_size;

		});
	IMPORT_VALUE_PLUS(param_type,
		{
			for(int i=0;i<3;i++)
				tone[i].param_type=param_type;
		});

	IMPORT_VALUE_PLUS(param_color[0],sync());
	IMPORT_VALUE_PLUS(param_color[1],sync());
	IMPORT_VALUE_PLUS(param_color[2],sync());

	IMPORT_VALUE_PLUS(param_subtractive,sync());
	for(int i=0;i<3;i++)
		if (param==strprintf("tone[%d].angle",i)
			&& tone[i].param_angle.get_type()==value.get_type())
		{
			tone[i].param_angle=value;
			return true;
		}
	for(int i=0;i<3;i++)
		if ((param==strprintf("tone[%d].origin",i) || param==strprintf("tone[%d].offset",i))
			&& tone[i].param_origin.get_type()==value.get_type())
		{
			tone[i].param_origin=value;
			return true;
		}

	return Layer_Composite::set_param(param,value);
}

ValueBase
Halftone3::get_param(const String & param)const
{
	EXPORT_VALUE(param_size);
	EXPORT_VALUE(param_type);

	EXPORT_VALUE(param_color[0]);
	EXPORT_VALUE(param_color[1]);
	EXPORT_VALUE(param_color[2]);

	EXPORT_VALUE(param_subtractive);
	for(int i=0;i<3;i++)
		if (param==strprintf("tone[%d].angle",i))
			return tone[i].param_angle;
	for(int i=0;i<3;i++)
		if (param==strprintf("tone[%d].origin",i))
			return tone[i].param_origin;

	EXPORT_NAME();
	EXPORT_VERSION();

	return Layer_Composite::get_param(param);
}

Layer::Vocab
Halftone3::get_param_vocab()const
{
	Layer::Vocab ret(Layer_Composite::get_param_vocab());

	ret.push_back(ParamDesc("size")
		.set_local_name(_("Mask Size"))
		.set_is_distance()
	);
	ret.push_back(ParamDesc("type")
		.set_local_name(_(" Type"))
		.set_hint("enum")
		.set_static(true)
		.add_enum_value(TYPE_SYMMETRIC,"symmetric",_("Symmetric"))
		.add_enum_value(TYPE_LIGHTONDARK,"lightondark",_("Light On Dark"))
		//.add_enum_value(TYPE_DARKONLIGHT,"darkonlight",_("Dark on Light"))
		.add_enum_value(TYPE_DIAMOND,"diamond",_("Diamond"))
		.add_enum_value(TYPE_STRIPE,"stripe",_("Stripe"))
	);
	ret.push_back(ParamDesc("subtractive")
		.set_local_name(_("Subtractive Flag"))
	);

	for(int i=0;i<3;i++)
	{
		String chan_name(strprintf("Chan%d",i));

		ret.push_back(ParamDesc(strprintf("color[%d]",i))
			.set_local_name(chan_name+_(" Color"))
		);

		ret.push_back(ParamDesc(strprintf("tone[%d].origin",i))
			.set_local_name(chan_name+_(" Mask Origin"))
			.set_is_distance()
		);
		ret.push_back(ParamDesc(strprintf("tone[%d].angle",i))
			.set_local_name(chan_name+_(" Mask Angle"))
			.set_origin(strprintf("tone[%d].origin",i))
		);
	}

	return ret;
}

Color
Halftone3::get_color(Context context, const Point &point)const
{
	const Color undercolor(context.get_color(point));
	const Color color(color_func(point,0,undercolor));

	if(get_amount()==1.0 && get_blend_method()==Color::BLEND_STRAIGHT)
		return color;
	else
		return Color::blend(color,undercolor,get_amount(),get_blend_method());
}

rendering::Task::Handle
Halftone3::build_composite_fork_task_vfunc(ContextParams /* context_params */, rendering::Task::Handle sub_task) const
{
	if (!sub_task)
		return sub_task;

	TaskHalfTone3::Handle task_halftone3(new TaskHalfTone3());
	for (int i=0; i < 3; i++)
		task_halftone3->color[i] = param_color[i].get(Color());
	task_halftone3->subtractive = param_subtractive.get(bool());
	for (int i=0; i < 3; i++)
		task_halftone3->tone[i] = tone[i];
	for (int i=0; i < 3; i++)
		for (int j=0; j < 3; j++)
			task_halftone3->inverse_matrix[i][j] = inverse_matrix[i][j];
	task_halftone3->sub_task() = sub_task;//->clone_recursive();

	return task_halftone3;
}
