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

#include "Common.h"
#include "TreeItemComponentDefault.h"
#include "TreeItem.h"
#include "LayerTreeItem.h"
#include "ProjectTreeItem.h"
#include "PianoLayerTreeItem.h"
#include "AutomationLayerTreeItem.h"
#include "LayerGroupTreeItem.h"
#include "Icons.h"
#include "TreeItemMarkerDefault.h"
#include "TreeItemMenuButton.h"
#include "HelioCallout.h"
#include "HelioTheme.h"

class TreeItemComponentFrame : public Component
{
public:

    TreeItemComponentFrame()
    {
        this->setInterceptsMouseClicks(false, false);
    }

    void paint(Graphics &g) override
    {
        const Colour colour = Colours::black.withAlpha(0.35f);
        const Rectangle<float> r(this->getLocalBounds()
                                 .reduced(-1, 4)
                                 .translated(4, 0)
                                 .toFloat());
        
        HelioTheme::drawDashedRectangle(g, r, colour, 5.5, 6.5, 0.3f, 7);
    }
};

TreeItemComponentDefault::TreeItemComponentDefault(TreeItem &i) :
    TreeItemComponent(i),
    textX(0.f),
    itemIsSelected(false),
    markerIsVisible(false)
{
    //this->selectionFrame = new TreeItemComponentFrame();
    //this->addChildComponent(this->selectionFrame);
    
    // Component(); // TreeItemComponentFrame(); //TreeItemMarkerDefault();
    const bool needsMarker = (nullptr != dynamic_cast<LayerTreeItem *>(&this->item));
    this->pageMarker = needsMarker ? new TreeItemMarkerDefault() : new Component();
    this->addChildComponent(this->pageMarker);
    
    this->menuButton = new TreeItemMenuButton();
    
    if (ScopedPointer<Component> hasMenu = this->item.createItemMenu())
    {
        this->menuButton->setAlpha(0.3f);
        this->addAndMakeVisible(this->menuButton);
    }
}

//===----------------------------------------------------------------------===//
// Component
//===----------------------------------------------------------------------===//

void TreeItemComponentDefault::paint(Graphics &g)
{
    this->paintIcon(g);

    g.setColour(Colours::white);
    const int menuButtonOffset = this->menuButton->isVisible() ? (this->menuButton->getWidth() + 4) : 0;
    
    this->paintText(g, Rectangle<float>(this->textX,
                                        0.f,
                                        float(this->getWidth()) - this->textX - menuButtonOffset,
                                        float(this->getHeight())));

    const bool selectionChanged = (this->item.isSelected() != this->itemIsSelected);
    this->itemIsSelected = this->item.isSelected();

    if (selectionChanged)
    {
        if (this->itemIsSelected)
        {
            this->menuButton->setAlpha(0.75f);
            //this->animator.fadeIn(this->selectionFrame, 250);
            //this->shiftTextRight();
        }
        else
        {
            this->menuButton->setAlpha(0.3f);
            //this->selectionFrame->setVisible(false);
            //this->animator.fadeOut(this->selectionFrame, 250);
            //this->shiftTextLeft();
        }
    }

    const bool markerVisibilityChanged = (this->item.isMarkerVisible() != this->markerIsVisible);
    this->markerIsVisible = this->item.isMarkerVisible();

    if (markerVisibilityChanged)
    {
        if (this->markerIsVisible)
        {
            this->animator.fadeIn(this->pageMarker, 150);
        }
        else
        {
            this->pageMarker->setVisible(false);
        }
    }
}

void TreeItemComponentDefault::resized()
{
    this->pageMarker->setBounds(this->getLocalBounds());
    //this->selectionFrame->setBounds(this->getLocalBounds());
    this->menuButton->setBounds(this->getLocalBounds().removeFromRight(this->getHeight()).translated(-2, 0));
    this->textX = float(this->item.getItemHeight());
}

void TreeItemComponentDefault::paintIcon(Graphics &g)
{
    Image icon(this->item.getIcon());
    const int cx = int(this->getHeight() / 2) + 2;
    const int cy = int(this->getHeight() / 2);
    Icons::drawImageRetinaAware(icon, g, cx, cy);
}

void TreeItemComponentDefault::paintText(Graphics &g, const Rectangle<float> &area)
{
    float alpha = 1.f; //0.9f;

    if (this->item.isGreyedOut())
    {
        alpha = 0.5f;
    }

    g.setFont(this->item.getFont());
    g.setColour(this->getItemColour().withMultipliedAlpha(alpha));
    g.drawText(this->item.getCaption(), area, Justification::centredLeft, false);

    // Looks no good, and we need to redraw it on state change
    //if (ProjectTreeItem *pti = dynamic_cast<ProjectTreeItem *>(&this->item))
    //{
    //    if (pti->getDocument()->hasUnsavedChanges())
    //    {
    //        g.drawText("*", area.translated(-10, 0.f), Justification::centredRight, false);
    //    }
    //}
    
    if (LayerTreeItem *lti = dynamic_cast<LayerTreeItem *>(&this->item))
    {
        if (lti->isMuted())
        {
            const int cY = int(area.getCentreY()) + 2;
            g.drawLine(int(area.getX()), cY, int(area.getX()) + this->item.getFont().getStringWidth(this->item.getName()), cY, 1.f);
        }
    }
}

void TreeItemComponentDefault::paintBackground(Graphics &g,
                                               int width, int height,
                                               bool isSelected, bool isActive)
{
    if (isSelected || isActive)
    {
        g.fillAll(Colours::white.withAlpha(0.05f));
    }
    
    g.fillAll(Colours::white.withAlpha(0.01f));
    
    g.setColour(Colours::white.withAlpha(6.f / 255.f));
    g.drawHorizontalLine(0, -500, 500);
    
    g.setColour(Colours::black.withAlpha(15.f / 255.f));
    g.drawHorizontalLine(height - 1, -500, 500);
}

//===----------------------------------------------------------------------===//
// LongTapListener
//===----------------------------------------------------------------------===//

void TreeItemComponentDefault::longTapEvent(const MouseEvent &e)
{
    
}

//===----------------------------------------------------------------------===//
// HighlightedComponent
//===----------------------------------------------------------------------===//

Component *TreeItemComponentDefault::createHighlighterComponent()
{
    //return new TreeItemComponentFrame();
    return new TreeItemComponentDefault(this->item);
}
