{
	header = 26

	ub[1]	placeFlagHasClipActions
	ub[1]	placeFlagHasClipDepth
	ub[1]	placeFlagHasName
	ub[1]	placeFlagHasRatio
	ub[1]	placeFlagHasColorTransform
	ub[1]	placeFlagHasMatrix
	ub[1]	placeFlagHasCharacter
	ub[1]	placeFlagHasMove

	u16		depth

	if placeFlagHasCharacter 		{ u16				characterId 	}
	if placeFlagHasMatrix 	 		{ Matrix			matrix 			}
	if placeFlagHasColorTransform 	{ ColorTransformA	colorTransform 	}
	if placeFlagHasRatio 			{ u16				ratio 			}
	if placeFlagHasName 			{ string			name 			}
	if placeFlagHasClipDepth 		{ u16				clipDepth 		}
	if placeFlagHasClipActions 		{ ClipActions		clipActions 	}
}