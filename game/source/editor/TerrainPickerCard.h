#pragma once

// Shared M95/M96 asset-picker card geometry. One hit target covers the
// thumbnail plus the name band. Not a generalized picker framework.

namespace editor
{
inline constexpr float kTerrainPickerCardNameLines = 2.0f;

inline float TerrainPickerCardHeight(float thumbSize, float textLineHeight)
{
    return thumbSize + textLineHeight * kTerrainPickerCardNameLines;
}

inline bool TerrainPickerCardContains(
    float pointerX,
    float pointerY,
    float cardMinX,
    float cardMinY,
    float cardWidth,
    float cardHeight)
{
    return pointerX >= cardMinX && pointerY >= cardMinY && pointerX < cardMinX + cardWidth
        && pointerY < cardMinY + cardHeight;
}

// True when a press lands on the thumbnail or the visible name band of one
// selectable card. A single activation; callers must not also fire a separate
// name-only control for the same click.
inline bool TerrainPickerCardShouldActivate(
    bool pointerPressed,
    float pointerX,
    float pointerY,
    float cardMinX,
    float cardMinY,
    float thumbSize,
    float textLineHeight)
{
    if (!pointerPressed || thumbSize <= 0.0f || textLineHeight < 0.0f)
    {
        return false;
    }
    return TerrainPickerCardContains(
        pointerX,
        pointerY,
        cardMinX,
        cardMinY,
        thumbSize,
        TerrainPickerCardHeight(thumbSize, textLineHeight));
}
}
