/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

//[Headers]
#include "Common.h"
//[/Headers]

#include "LighterShadowDownwards.h"

//[MiscUserDefs]
//[/MiscUserDefs]

LighterShadowDownwards::LighterShadowDownwards()
{

    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 40);

    //[Constructor]
    this->setInterceptsMouseClicks(false, false);
    //[/Constructor]
}

LighterShadowDownwards::~LighterShadowDownwards()
{
    //[Destructor_pre]
    //[/Destructor_pre]


    //[Destructor]
    //[/Destructor]
}

void LighterShadowDownwards::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.setGradientFill (ColourGradient (Colour (0x09000000),
                                       0.0f, 0.0f,
                                       Colour (0x00000000),
                                       0.0f, static_cast<float> (getHeight()),
                                       false));
    g.fillRect (0, 0, getWidth() - 0, getHeight() - 0);

    g.setGradientFill (ColourGradient (Colour (0x09000000),
                                       0.0f, 0.0f,
                                       Colour (0x00000000),
                                       0.0f, static_cast<float> (proportionOfHeight (0.5000f)),
                                       false));
    g.fillRect (0, 0, getWidth() - 0, proportionOfHeight (0.5000f));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void LighterShadowDownwards::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}


//[MiscUserCode]
//[/MiscUserCode]

#if 0
/*
BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="LighterShadowDownwards" template="../../Template"
                 componentName="" parentClasses="public Component" constructorParams=""
                 variableInitialisers="" snapPixels="8" snapActive="1" snapShown="1"
                 overlayOpacity="0.330" fixedSize="1" initialWidth="600" initialHeight="40">
  <BACKGROUND backgroundColour="0">
    <RECT pos="0 0 0M 0M" fill="linear: 0 0, 0 0R, 0=9000000, 1=0" hasStroke="0"/>
    <RECT pos="0 0 0M 50%" fill="linear: 0 0, 0 50%, 0=9000000, 1=0" hasStroke="0"/>
  </BACKGROUND>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
