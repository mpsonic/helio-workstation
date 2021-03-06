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
#include "MidiRollHeader.h"
#include "MidiRoll.h"
#include "MidiLayer.h"
#include "MidiLayerOwner.h"
#include "Transport.h"
#include "AnnotationsTrackMap.h"
#include "Origami.h"
#include "SoundProbeIndicator.h"
#include "TimeDistanceIndicator.h"
#include "MidiEventComponentLasso.h"
#include "HeaderSelectionIndicator.h"
#include "CommandIDs.h"

#define MIDIROLL_HEADER_ALIGNS_TO_BEATS 1
#define MIDIROLL_HEADER_SELECTION_ALIGNS_TO_BEATS 0
#define MIN_TIME_DISTANCE_INDICATOR_SIZE (40)

MidiRollHeader::MidiRollHeader(Transport &transportRef, MidiRoll &rollRef, Viewport &viewportRef) :
    transport(transportRef),
    roll(rollRef),
    viewport(viewportRef),
    isActive(false),
    soundProbeMode(false)
{
    this->setOpaque(true);
    this->setAlwaysOnTop(true);
    this->setSize(this->getParentWidth(), MIDIROLL_HEADER_HEIGHT);
}

MidiRollHeader::~MidiRollHeader()
{
}

void MidiRollHeader::setSoundProbeMode(bool shouldPlayOnClick)
{
    if (this->soundProbeMode == shouldPlayOnClick)
    {
        return;
    }
    
    this->soundProbeMode = shouldPlayOnClick;
    
    if (this->soundProbeMode)
    {
        this->setMouseCursor(MouseCursor::PointingHandCursor);
    }
    else
    {
        this->pointingIndicator = nullptr;
        this->timeDistanceIndicator = nullptr;
        this->setMouseCursor(MouseCursor::NormalCursor);
    }
}

void MidiRollHeader::setActive(bool shouldBeActive)
{
    this->isActive = shouldBeActive;
    this->repaint();
}

void MidiRollHeader::updateIndicatorPosition(SoundProbeIndicator *indicator, const MouseEvent &e)
{
    indicator->setAnchoredAt(this->getAlignedAnchorForEvent(e));
}

float MidiRollHeader::getUnalignedAnchorForEvent(const MouseEvent &e) const
{
    const MouseEvent parentEvent = e.getEventRelativeTo(&this->roll);
    const float absX = float(parentEvent.getPosition().getX()) / float(this->roll.getWidth());
    return absX;
}

float MidiRollHeader::getAlignedAnchorForEvent(const MouseEvent &e) const
{
    const MouseEvent parentEvent = e.getEventRelativeTo(&this->roll);
    
#if MIDIROLL_HEADER_ALIGNS_TO_BEATS
    const float roundBeat = this->roll.getRoundBeatByXPosition(parentEvent.x);
    const int roundX = this->roll.getXPositionByBeat(roundBeat);
    const float absX = float(roundX) / float(this->roll.getWidth());
#else
    const float absX = this->getUnalignedAnchorForEvent(e);
#endif
    
    return absX;
}

void MidiRollHeader::updateTimeDistanceIndicator()
{
    if (this->pointingIndicator == nullptr ||
        this->playingIndicator == nullptr ||
        this->timeDistanceIndicator == nullptr)
    {
        return;
    }
    
    const float anchor1 = this->pointingIndicator->getAnchor();
    const float anchor2 = this->playingIndicator->getAnchor();
    
    const double seek1 = this->roll.getTransportPositionByXPosition(this->pointingIndicator->getX(), this->getWidth());
    const double seek2 = this->roll.getTransportPositionByXPosition(this->playingIndicator->getX(), this->getWidth());

    this->timeDistanceIndicator->setAnchoredBetween(anchor1, anchor2);
    
    double outTimeMs1 = 0.0;
    double outTempo1 = 0.0;
    double outTimeMs2 = 0.0;
    double outTempo2 = 0.0;
    
    // todo dont rebuild sequences here
    this->transport.calcTimeAndTempoAt(seek1, outTimeMs1, outTempo1);
    this->transport.calcTimeAndTempoAt(seek2, outTimeMs2, outTempo2);
    
    const double timeDelta = fabs(outTimeMs2 - outTimeMs1);
    const String timeDeltaText = Transport::getTimeString(timeDelta);
    this->timeDistanceIndicator->getTimeLabel()->setText(timeDeltaText, dontSendNotification);
}


//===----------------------------------------------------------------------===//
// Component
//===----------------------------------------------------------------------===//

void MidiRollHeader::mouseDown(const MouseEvent &e)
{
    if (this->soundProbeMode)
    {
        // todo if playing, dont probe anything?
        
#if MIDIROLL_HEADER_ALIGNS_TO_BEATS
        const float roundBeat = this->roll.getRoundBeatByXPosition(e.x);
        const double transportPosition = this->roll.getTransportPositionByBeat(roundBeat);
#else
        const double transportPosition = this->roll.getTransportPositionByXPosition(e.x, float(this->getWidth()));
#endif
        
        const bool shouldProbeAllLayers = (!e.mods.isAnyModifierKeyDown() || e.mods.isRightButtonDown());
        
        if (shouldProbeAllLayers)
        {
            this->transport.probeSoundAt(transportPosition, nullptr);
        }
        else
        {
            this->transport.probeSoundAt(transportPosition, this->roll.getPrimaryActiveMidiLayer());
        }
        
        this->playingIndicator = new SoundProbeIndicator();
        this->roll.addAndMakeVisible(this->playingIndicator);
        this->updateIndicatorPosition(this->playingIndicator, e);
    }
    else
    {
#if MIDIROLL_HEADER_ALIGNS_TO_BEATS
        const MouseEvent parentEvent = e.getEventRelativeTo(&this->roll);
        const float roundBeat = this->roll.getRoundBeatByXPosition(e.x); // skipped e.getEventRelativeTo(*this->roll);
        const double transportPosition = this->roll.getTransportPositionByBeat(roundBeat);
#else
        const double transportPosition = this->roll.getTransportPositionByXPosition(e.x, float(this->getWidth()));
#endif
        
        const bool shouldStartSelection = (e.mods.isAltDown() ||
                                           e.mods.isCommandDown() ||
                                           e.mods.isCtrlDown() ||
                                           e.mods.isShiftDown() ||
                                           this->roll.isInSelectionMode());
        
        if (shouldStartSelection)
        {
            const MouseEvent parentEvent = e.getEventRelativeTo(&this->roll);
            
#if MIDIROLL_HEADER_SELECTION_ALIGNS_TO_BEATS
            const float roundBeat = this->roll.getRoundBeatByXPosition(parentEvent.x);
            const int roundX = this->roll.getXPositionByBeat(roundBeat);
			const float newX = float(roundX + 1);
#else
            const float newX = parentEvent.position.x;
#endif

            const MouseEvent e2(e.source,
                                Point<float>(newX, 0.f),
                                e.mods,
                                1.f,
                                0.0f, 0.0f, 0.0f, 0.0f,
                                e.originalComponent,
                                e.originalComponent,
                                Time::getCurrentTime(),
                                Point<float>(newX, 0.f),
                                Time::getCurrentTime(),
                                1,
                                false);
            
            this->roll.getLasso()->beginLasso(e2, &this->roll);
            
            this->selectionIndicator = new HeaderSelectionIndicator();
            this->addAndMakeVisible(this->selectionIndicator);
            this->selectionIndicator->setBounds(0, this->getHeight() - this->selectionIndicator->getHeight(),
                                                0, this->selectionIndicator->getHeight());
            
#if MIDIROLL_HEADER_SELECTION_ALIGNS_TO_BEATS
			this->selectionIndicator->setStartAnchor(this->getAlignedAnchorForEvent(e));
#else
			this->selectionIndicator->setStartAnchor(this->getUnalignedAnchorForEvent(e));
#endif
        }
        else
        {
            this->transport.stopPlayback();
            this->roll.cancelPendingUpdate(); // why is it here?
            this->transport.seekToPosition(transportPosition);
        }
    }
}

void MidiRollHeader::mouseDrag(const MouseEvent &e)
{
    if (this->soundProbeMode)
    {
        if (this->pointingIndicator != nullptr)
        {
            this->updateIndicatorPosition(this->pointingIndicator, e);

            if (this->playingIndicator != nullptr)
            {
                const int distance = abs(this->pointingIndicator->getX() - this->playingIndicator->getX());

                if (this->timeDistanceIndicator == nullptr)
                {
                    //Logger::writeToLog("MidiRollHeader::initTimeDistanceIndicatorIfPossible " + String(distance));
                    
                    // todo rebuild sequences if not playing, do nothing if playing
                    this->transport.stopPlayback();
                    
                    if (distance > MIN_TIME_DISTANCE_INDICATOR_SIZE)
                    {
                        this->timeDistanceIndicator = new TimeDistanceIndicator();
                        this->roll.addAndMakeVisible(this->timeDistanceIndicator);
                        this->timeDistanceIndicator->setBounds(0, this->getBottom() + 4,
                                                               0, this->timeDistanceIndicator->getHeight());
                        this->updateTimeDistanceIndicator();
                    }
                }
                else
                {
                    if (distance <= MIN_TIME_DISTANCE_INDICATOR_SIZE)
                    {
                        this->timeDistanceIndicator = nullptr;
                    }
                    else
                    {
                        this->updateTimeDistanceIndicator();
                    }
                }
            }
        }
    }
    else
    {
        if (this->roll.getLasso()->isDragging())
        {
            const MouseEvent parentEvent = e.getEventRelativeTo(&this->roll);
            
#if MIDIROLL_HEADER_SELECTION_ALIGNS_TO_BEATS
            const float roundBeat = this->roll.getRoundBeatByXPosition(parentEvent.x);
            const int roundX = this->roll.getXPositionByBeat(roundBeat);
            const MouseEvent parentGlobalSelection = parentEvent.withNewPosition(Point<int>(roundX - 1, this->roll.getHeight()));
#else
            const MouseEvent parentGlobalSelection = parentEvent.withNewPosition(Point<int>(parentEvent.x, this->roll.getHeight()));
#endif
            
            this->roll.getLasso()->dragLasso(parentGlobalSelection);
            
            if (this->selectionIndicator != nullptr)
            {
#if MIDIROLL_HEADER_SELECTION_ALIGNS_TO_BEATS
				this->selectionIndicator->setEndAnchor(this->getAlignedAnchorForEvent(e));
#else
				this->selectionIndicator->setEndAnchor(this->getUnalignedAnchorForEvent(e));
#endif
            }
        }
        else
        {
            //if (! this->transport.isPlaying())
            {
#if MIDIROLL_HEADER_ALIGNS_TO_BEATS
                const float roundBeat = this->roll.getRoundBeatByXPosition(e.x); // skipped e.getEventRelativeTo(*this->roll);
                const double transportPosition = this->roll.getTransportPositionByBeat(roundBeat);
#else
                const double transportPosition = this->roll.getTransportPositionByXPosition(e.x, float(this->getWidth()));
#endif
                
                this->transport.stopPlayback();
                this->roll.cancelPendingUpdate();
                this->transport.seekToPosition(transportPosition);
            }
        }
    }
}

void MidiRollHeader::mouseUp(const MouseEvent &e)
{
    this->playingIndicator = nullptr;
    this->timeDistanceIndicator = nullptr;
    this->selectionIndicator = nullptr;
    
    if (this->soundProbeMode)
    {
        this->transport.allNotesControllersAndSoundOff();
        return;
    }
    
    if (this->roll.getLasso()->isDragging())
    {
        this->roll.getLasso()->endLasso();
    }
    else
    {
        if (this->transport.isPlaying())
        {
            this->transport.stopPlayback();
        }
        
#if MIDIROLL_HEADER_ALIGNS_TO_BEATS
        const float roundBeat = this->roll.getRoundBeatByXPosition(e.x); // skipped e.getEventRelativeTo(*this->roll);
        const double transportPosition = this->roll.getTransportPositionByBeat(roundBeat);
        //Logger::writeToLog("Click beat: " + String(roundBeat) + ", transport position: " + String(transportPosition));

#else
        const double transportPosition = this->roll.getTransportPositionByXPosition(e.x, float(this->getWidth()));
#endif
        
        this->transport.seekToPosition(transportPosition);
        
        if (e.mods.isRightButtonDown())
        {
            this->transport.startPlayback();
        }
    }
}

void MidiRollHeader::mouseEnter(const MouseEvent &e)
{
}

void MidiRollHeader::mouseMove(const MouseEvent &e)
{
    if (this->pointingIndicator != nullptr)
    {
        this->updateIndicatorPosition(this->pointingIndicator, e);
    }
    
    if (this->soundProbeMode)
    {
        if (this->pointingIndicator == nullptr)
        {
            this->pointingIndicator = new SoundProbeIndicator();
            this->roll.addAndMakeVisible(this->pointingIndicator);
            this->updateIndicatorPosition(this->pointingIndicator, e);
        }
    }
}

void MidiRollHeader::mouseExit(const MouseEvent &e)
{
    if (this->pointingIndicator != nullptr)
    {
        this->pointingIndicator = nullptr;
    }
    
    if (this->timeDistanceIndicator != nullptr)
    {
        this->timeDistanceIndicator = nullptr;
    }
}

void MidiRollHeader::mouseDoubleClick(const MouseEvent &e)
{
	// TODO instead show a menu like "add annotation / add time signature"?
    this->roll.postCommandMessage(CommandIDs::AddAnnotation);
}

void MidiRollHeader::paint(Graphics &g)
{
    const int paintStartX = this->viewport.getViewPositionX();
    const int paintEndX = this->viewport.getViewPositionX() + this->viewport.getViewWidth();

    const Colour backCol(this->findColour(MidiRoll::headerColourId));
    const Colour frontCol(this->isActive ?
                          backCol.contrasting().withMultipliedAlpha(0.2f) :
                          backCol.contrasting().withMultipliedAlpha(0.1f));

    g.setColour(backCol);
    g.fillRect(paintStartX, 0, paintEndX - paintStartX, MIDIROLL_HEADER_HEIGHT);

    //HelioTheme::drawNoise(g, getWidth(), getHeight(), 3);
    
    g.setColour(frontCol);
    
    for (const auto f : this->roll.getVisibleBars())
    {
        g.drawLine(f, float(this->getHeight() - 16), f, float(this->getHeight() - 1), 2.5f);
    }

    for (const auto f : this->roll.getVisibleBeats())
    {
        g.drawVerticalLine(f, float(this->getHeight() - 10), float(this->getHeight() - 1.f));
    }

    for (const auto f : this->roll.getVisibleSnaps())
    {
        g.drawVerticalLine(f, float(this->getHeight() - 4), float(this->getHeight() - 1.f));
    }

    g.setColour(Colours::white.withAlpha(0.025f));
    g.drawHorizontalLine(this->getHeight() - 2, 0.f, float(this->getWidth()));
    //g.drawHorizontalLine(0, 0.f, float(this->getWidth()));

    g.setColour(Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine(this->getHeight() - 1, 0.f, float(this->getWidth()));
}
