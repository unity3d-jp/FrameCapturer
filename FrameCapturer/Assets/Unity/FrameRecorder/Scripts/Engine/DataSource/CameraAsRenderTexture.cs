using System;
using UnityEditor;
using UnityEngine.Recorder.FrameRecorder.Utilities;
using UnityEngine.Rendering;

namespace UnityEngine.Recorder.FrameRecorder.DataSource
{
    public class CameraAsRenderTexture : RenderTextureSource
    {
        Shader          m_shCopy;
        Material        m_mat_copy;
        Mesh            m_quad;
        CommandBuffer   m_cbCopyFB;
        CommandBuffer   m_cbCopyGB;
        CommandBuffer   m_cbClearGB;
        CommandBuffer   m_cbCopyVelocity;
        Camera          m_Camera;
        bool            m_cameraChanged;
        int             m_Width;

        public Camera TargetCamera
        {
            get { return m_Camera; }

            set
            {
                if (m_Camera != value)
                {
                    ReleaseCamera();
                    m_Camera = value;
                    m_cameraChanged = true;
                }
            }
        }

        public Shader CopyShader
        {
            get
            {
                if (m_shCopy == null)
                {
                    // TODO: make this Non Editor compatible
                    // Figure out where this shader should reside.
                    m_shCopy = AssetDatabase.LoadAssetAtPath<Shader>(AssetDatabase.GUIDToAssetPath("2283fb92223c7914c9096670e29202c8"));
                }
                return m_shCopy;
            }

            set { m_shCopy = value; }
        }

        public CameraAsRenderTexture(int width)
        {
            m_Width = width;
            m_quad = CreateFullscreenQuad();
        }

        public void PrepareNewFrame()
        {
            // initialize scratch buffer
            var newTexture = PrepFrameRenderTexture();

            // initialize command buffer
            if (m_Camera != null && m_cameraChanged || newTexture)
            {
                if (m_cbCopyFB != null)
                {
                    m_Camera.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cbCopyFB);
                    m_cbCopyFB.Release();
                }

                // TODO: This should not be here!!!
                m_mat_copy = new Material(CopyShader);
                if (m_Camera.targetTexture != null)
                    m_mat_copy.EnableKeyword("OFFSCREEN");

                var tid = Shader.PropertyToID("_TmpFrameBuffer");
                m_cbCopyFB = new CommandBuffer { name = "Frame Recorder: copy frame buffer" };
                m_cbCopyFB.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Bilinear);
                m_cbCopyFB.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                m_cbCopyFB.SetRenderTarget(buffer);
                m_cbCopyFB.DrawMesh(m_quad, Matrix4x4.identity, m_mat_copy, 0, 0);
                m_cbCopyFB.ReleaseTemporaryRT(tid);
                m_Camera.AddCommandBuffer(CameraEvent.AfterEverything, m_cbCopyFB);

                m_cameraChanged = false;
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                ReleaseCamera();
            }

            base.Dispose(disposing);
        }

        protected virtual void ReleaseCamera()
        {
            if (m_cbCopyFB != null)
            {
                if (m_Camera != null)
                    m_Camera.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cbCopyFB);

                m_cbCopyFB.Release();
                m_cbCopyFB = null;
            }

            if (m_mat_copy != null)
                UnityHelpers.Destroy(m_mat_copy);
        }

        bool PrepFrameRenderTexture()
        {
            var height = (int)(m_Width / m_Camera.aspect);
            if (buffer != null)
            {
                if (buffer.IsCreated() && buffer.width == m_Width && buffer.height == height)
                {
                    return false;
                }

                ReleaseBuffer();
            }

            buffer = new RenderTexture(m_Width, height, 0, RenderTextureFormat.ARGB32)
            {
                wrapMode = TextureWrapMode.Repeat
            };
            buffer.Create();

            return true;
        }

        public static Mesh CreateFullscreenQuad()
        {
            var vertices = new Vector3[4]
            {
                new Vector3(1.0f, 1.0f, 0.0f),
                new Vector3(-1.0f, 1.0f, 0.0f),
                new Vector3(-1.0f, -1.0f, 0.0f),
                new Vector3(1.0f, -1.0f, 0.0f),
            };
            var indices = new[] { 0, 1, 2, 2, 3, 0 };

            var r = new Mesh
            {
                vertices = vertices,
                triangles = indices
            };
            return r;
        }
    }
}
