/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
* Copyright 2008-2012 Pelican Mapping
* http://osgearth.org
*
* osgEarth is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*
* Original author: Thomas Lerman
*/
#include <osgEarthUtil/CMYKColorFilter>
#include <osgEarth/ShaderComposition>
#include <osgEarth/StringUtils>
#include <osgEarth/ThreadingUtils>
#include <osg/Program>
#include <OpenThreads/Atomic>

using namespace osgEarth;
using namespace osgEarth::Util;

namespace
{
    static OpenThreads::Atomic s_uniformNameGen;

    static const char* s_localShaderSource =
        "#version 110\n"
        "uniform vec4 __UNIFORM_NAME__;\n"

        "void __ENTRY_POINT__(in int slot, inout vec4 color)\n"
        "{\n"
        "    if ((__UNIFORM_NAME__.x != 0.0) || (__UNIFORM_NAME__.y != 0.0) || (__UNIFORM_NAME__.z != 0.0) || (__UNIFORM_NAME__.w != 0.0))\n"
        "    {\n"
        "        color.rgb = clamp(color.rgb - __UNIFORM_NAME__.xyz - __UNIFORM_NAME__.w, 0.0, 1.0); \n"
        "    }\n"
        "}\n";
}

//---------------------------------------------------------------------------

#define FUNCTION_PREFIX "osgearthutil_cmykColorFilter_"
#define UNIFORM_PREFIX  "osgearthutil_u_cmyk_"

//---------------------------------------------------------------------------

CMYKColorFilter::CMYKColorFilter(void)
{
    init();
}

void
CMYKColorFilter::init()
{
    // Generate a unique name for this filter's uniform. This is necessary
    // so that each layer can have a unique uniform and entry point.
    m_instanceId = (++s_uniformNameGen) - 1;
    m_cmyk = new osg::Uniform(osg::Uniform::FLOAT_VEC4, (osgEarth::Stringify() << UNIFORM_PREFIX << m_instanceId));
    m_cmyk->set(osg::Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
}

// CMY (without the K): http://forums.adobe.com/thread/428899
void CMYKColorFilter::setCMYKOffset(const osg::Vec4f& cmyk)
{
    m_cmyk->set(cmyk);
}

osg::Vec4f CMYKColorFilter::getCMYKOffset(void) const
{
    osg::Vec4f cmyk;
    m_cmyk->get(cmyk);
    return (cmyk);
}

void CMYKColorFilter::setRGBWOffset(const osg::Vec4f& rgbw)
{
    m_cmyk->set(-rgbw);
}

osg::Vec4f CMYKColorFilter::getRGBWOffset(void) const
{
    osg::Vec4f cmyk;
    m_cmyk->get(cmyk);
    return (-cmyk);
}

std::string CMYKColorFilter::getEntryPointFunctionName(void) const
{
    return (osgEarth::Stringify() << FUNCTION_PREFIX << m_instanceId);
}

void CMYKColorFilter::install(osg::StateSet* stateSet) const
{
    // safe: will not add twice.
    stateSet->addUniform(m_cmyk.get());

    osgEarth::VirtualProgram* vp = dynamic_cast<osgEarth::VirtualProgram*>(stateSet->getAttribute(osg::StateAttribute::PROGRAM));
    if (vp)
    {
        // build the local shader (unique per instance). We will
        // use a template with search and replace for this one.
        std::string entryPoint = osgEarth::Stringify() << FUNCTION_PREFIX << m_instanceId;
        std::string code = s_localShaderSource;
        osgEarth::replaceIn(code, "__UNIFORM_NAME__", m_cmyk->getName());
        osgEarth::replaceIn(code, "__ENTRY_POINT__", entryPoint);

        osg::Shader* main = new osg::Shader(osg::Shader::FRAGMENT, code);
        //main->setName(entryPoint);
        vp->setShader(entryPoint, main);
    }
}


//---------------------------------------------------------------------------

OSGEARTH_REGISTER_COLORFILTER( cmyk, osgEarth::Util::CMYKColorFilter );


CMYKColorFilter::CMYKColorFilter(const Config& conf)
{
    init();

    osg::Vec4f val;
    val[0] = conf.value("c", 0.0);
    val[1] = conf.value("m", 0.0);
    val[2] = conf.value("y", 0.0);
    val[3] = conf.value("k", 0.0);
    setCMYKOffset( val );
}

Config
CMYKColorFilter::getConfig() const
{
    osg::Vec4f val = getCMYKOffset();
    Config conf("cmyk");
    conf.add( "c", val[0] );
    conf.add( "m", val[1] );
    conf.add( "y", val[2] );
    conf.add( "k", val[3] );
    return conf;
}
