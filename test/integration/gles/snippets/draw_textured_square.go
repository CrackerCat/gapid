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

package snippets

import (
	"context"

	"github.com/google/gapid/gapis/api"
	"github.com/google/gapid/gapis/api/gles"
)

// DrawTexturedSquare returns the command list needed draw a textured square.
func (b *Builder) DrawTexturedSquare(ctx context.Context) (draw, swap api.CmdID) {
	squareVertices := []float32{
		-0.5, -0.5, 0.5,
		-0.5, +0.5, 0.5,
		+0.5, +0.5, 0.5,
		+0.5, -0.5, 0.5,
	}

	squareIndices := []uint16{
		0, 1, 2, 0, 2, 3,
	}

	textureVSSource := `
		precision mediump float;
		attribute vec3 position;
		varying vec2 texcoord;
		void main() {
			gl_Position = vec4(position, 1.0);
			texcoord = position.xy + vec2(0.5, 0.5);
		}`

	textureFSSource := `
		precision mediump float;
		uniform sampler2D tex;
		varying vec2 texcoord;
		void main() {
			gl_FragColor = texture2D(tex, texcoord);
		}`

	vs, fs, prog, pos := b.newShaderID(), b.newShaderID(), b.newProgramID(), gles.AttributeLocation(0)
	texLoc := gles.UniformLocation(0)

	textureNames := []gles.TextureId{1}
	textureNamesPtr := b.data(ctx, textureNames)
	texData := make([]uint8, 3*64*64)
	for y := 0; y < 64; y++ {
		for x := 0; x < 64; x++ {
			texData[y*64*3+x*3] = uint8(x * 4)
			texData[y*64*3+x*3+1] = uint8(y * 4)
			texData[y*64*3+x*3+2] = 255
		}
	}

	textureData := b.data(ctx, texData)
	squareIndicesPtr := b.data(ctx, squareIndices)
	squareVerticesPtr := b.data(ctx, squareVertices)

	// Build the program resource
	b.program(ctx, vs, fs, prog, textureVSSource, textureFSSource)

	uniformTex := gles.MakeProgramResourceʳ()
	uniformTex.SetType(gles.GLenum_GL_SAMPLER_2D)
	uniformTex.SetName("tex")
	uniformTex.SetArraySize(1)
	uniformTex.Locations().Add(0, gles.GLint(texLoc))

	positionIn := gles.MakeProgramResourceʳ()
	positionIn.SetType(gles.GLenum_GL_FLOAT_VEC3)
	positionIn.SetName("position")
	positionIn.SetArraySize(1)
	positionIn.Locations().Add(0, gles.GLint(pos))

	resources := gles.MakeActiveProgramResourcesʳ()
	resources.DefaultUniformBlock().Add(0, uniformTex)
	resources.ProgramInputs().Add(0, positionIn)

	lpe := gles.MakeLinkProgramExtra()
	lpe.SetLinkStatus(gles.GLboolean_GL_TRUE)
	lpe.SetActiveResources(resources)

	b.Add(
		api.WithExtras(b.cb.GlLinkProgram(prog), lpe),
		gles.GetUniformLocation(ctx, b.state, b.cb, prog, "tex", texLoc),
	)

	// Build the texture resource
	b.Add(
		b.cb.GlGenTextures(1, textureNamesPtr.Ptr()).AddWrite(textureNamesPtr.Data()),
		b.cb.GlBindTexture(gles.GLenum_GL_TEXTURE_2D, textureNames[0]),
		b.cb.GlTexParameteri(gles.GLenum_GL_TEXTURE_2D, gles.GLenum_GL_TEXTURE_MIN_FILTER, gles.GLint(gles.GLenum_GL_NEAREST)),
		b.cb.GlTexParameteri(gles.GLenum_GL_TEXTURE_2D, gles.GLenum_GL_TEXTURE_MAG_FILTER, gles.GLint(gles.GLenum_GL_NEAREST)),
		b.cb.GlTexImage2D(
			gles.GLenum_GL_TEXTURE_2D,
			0,
			gles.GLint(gles.GLenum_GL_RGB),
			64,
			64,
			0,
			gles.GLenum_GL_RGB,
			gles.GLenum_GL_UNSIGNED_BYTE,
			textureData.Ptr(),
		).AddRead(textureData.Data()),
	)

	// Render square using the build program and texture
	b.Add(
		b.cb.GlEnable(gles.GLenum_GL_DEPTH_TEST), // Required for depth-writing
		b.cb.GlClearColor(0.0, 1.0, 0.0, 1.0),
		b.cb.GlClear(gles.GLbitfield_GL_COLOR_BUFFER_BIT|gles.GLbitfield_GL_DEPTH_BUFFER_BIT),
		b.cb.GlUseProgram(prog),
		b.cb.GlActiveTexture(gles.GLenum_GL_TEXTURE0),
		b.cb.GlBindTexture(gles.GLenum_GL_TEXTURE_2D, textureNames[0]),
		b.cb.GlUniform1i(texLoc, 0),
		gles.GetAttribLocation(ctx, b.state, b.cb, prog, "position", pos),
		b.cb.GlEnableVertexAttribArray(pos),
		b.cb.GlVertexAttribPointer(pos, 3, gles.GLenum_GL_FLOAT, gles.GLboolean(0), 0, squareVerticesPtr.Ptr()),
		b.cb.GlDrawElements(gles.GLenum_GL_TRIANGLES, 6, gles.GLenum_GL_UNSIGNED_SHORT, squareIndicesPtr.Ptr()).
			AddRead(squareIndicesPtr.Data()).
			AddRead(squareVerticesPtr.Data()),
	)
	draw = b.Last()
	swap = b.SwapBuffers()

	return draw, swap
}