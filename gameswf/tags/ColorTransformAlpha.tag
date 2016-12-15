{
	ub[1]	hasAddTerms
	ub[1]	hasMultTerms
	ub[4]	nBits

	if hasMultTerms {
		sb[nBits]	redMultTerm
		sb[nBits]	greenMultTerm
		sb[nBits]	blueMultTerm
		sb[nBits]	alphaMultTerm
	}

	if hasAddTerms {
		sb[nBits]	redAddTerm
		sb[nBits]	greenAddTerm
		sb[nBits]	blueAddTerm
		sb[nBits]	alphaAddTerm
	}
}