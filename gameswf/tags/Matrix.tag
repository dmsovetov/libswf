{
	ub[1] hasScale

	if hasScale {
		ub[5]			nScaleBits
		fb[nScaleBits]	scaleX
		fb[nScaleBits]	scaleY
	}

	ub[1] hasRotate

	if hasRotate {
		ub[5]			nRotateBits
		fb[nRotateBits]	rotateSkew0
		fb[nRotateBits]	rotateSkew1
	}

	ub[5] 				nTranslateBits
	sb[nTranslateBits]	translateX
	sb[nTranslateBits]	translateY
}