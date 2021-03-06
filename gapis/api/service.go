// Copyright (C) 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package api

import "fmt"

// IsColor returns true if a is a color attachment.
func (a FramebufferAttachment) IsColor() bool {
	switch a {
	case FramebufferAttachment_Color0,
		FramebufferAttachment_Color1,
		FramebufferAttachment_Color2,
		FramebufferAttachment_Color3:
		return true
	default:
		return false
	}
}

// IsDepth returns true if a is a depth attachment.
func (a FramebufferAttachment) IsDepth() bool {
	return a == FramebufferAttachment_Depth
}

// IsStencil returns true if a is a stencil attachment.
func (a FramebufferAttachment) IsStencil() bool {
	return a == FramebufferAttachment_Stencil
}

func (a AspectType) Format(f fmt.State, c rune) {
	switch a {
	case AspectType_COLOR:
		fmt.Fprint(f, "Color")
	case AspectType_DEPTH:
		fmt.Fprint(f, "Depth")
	case AspectType_STENCIL:
		fmt.Fprint(f, "Stencil")
	default:
		fmt.Fprintf(f, "Unknown AspectType %d", int(a))
	}
}
