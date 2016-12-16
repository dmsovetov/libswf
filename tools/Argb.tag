Argb
{
	u8 alpha
	u8 red
	u8 green
	u8 blue
}

ClipActions
{
	u16					reserved
	ClipEventFlags		allEventFlags
	ClipActionRecord:*	clipActionRecords

	if version <= 5 {
		u16				clipActionEndFlag
	}
	if version >= 6 {
		u32				clipActionEndFlag
	}
}

ColorTransform
{
	ub:1	hasAddTerms
	ub:1	hasMultTerms
	ub:4	nBits

	if hasMultTerms {
		sb:nBits	redMultTerm
		sb:nBits	greenMultTerm
		sb:nBits	blueMultTerm
	}

	if hasAddTerms {
		sb:nBits	redAddTerm
		sb:nBits	greenAddTerm
		sb:nBits	blueAddTerm
	}
}

Matrix
{
	ub:1 hasScale

	if hasScale {
		ub:5			nScaleBits
		fb:nScaleBits	scaleX
		fb:nScaleBits	scaleY
	}

	ub:1 hasRotate

	if hasRotate {
		ub:5			nRotateBits
		fb:nRotateBits	rotateSkew0
		fb:nRotateBits	rotateSkew1
	}

	ub:5 				nTranslateBits
	sb:nTranslateBits	translateX
	sb:nTranslateBits	translateY
}

PlaceObject2
{
	ub:1	placeFlagHasClipActions
	ub:1	placeFlagHasClipDepth
	ub:1	placeFlagHasName
	ub:1	placeFlagHasRatio
	ub:1	placeFlagHasColorTransform
	ub:1	placeFlagHasMatrix
	ub:1	placeFlagHasCharacter
	ub:1	placeFlagHasMove

	u16		depth

	if placeFlagHasCharacter 		{ u16				characterId 	}
	if placeFlagHasMatrix 	 		{ Matrix			matrix 			}
	if placeFlagHasColorTransform 	{ ColorTransformA	colorTransform 	}
	if placeFlagHasRatio 			{ u16				ratio 			}
	if placeFlagHasName 			{ string			name 			}
	if placeFlagHasClipDepth 		{ u16				clipDepth 		}
	if placeFlagHasClipActions 		{ ClipActions		clipActions 	}
}