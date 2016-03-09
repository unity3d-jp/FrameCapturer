using System;
using UnityEngine;


namespace UTJ
{
    [Serializable]
    public class DataPath
    {
        public enum Root
        {
            CurrentDirectory,
            PersistentDataPath,
            StreamingAssetsPath,
            TemporaryCachePath,
            DataPath,
        }

        public Root m_root;
        public string m_leaf;

        public DataPath() { }
        public DataPath(Root root, string leaf)
        {
            m_root = root;
            m_leaf = leaf;
        }

        public string GetPath()
        {
            string ret = "";
            switch (m_root)
            {
                case Root.CurrentDirectory:
                    ret += ".";
                    break;
                case Root.PersistentDataPath:
                    ret += Application.persistentDataPath;
                    break;
                case Root.StreamingAssetsPath:
                    ret += Application.streamingAssetsPath;
                    break;
                case Root.TemporaryCachePath:
                    ret += Application.temporaryCachePath;
                    break;
                case Root.DataPath:
                    ret += Application.dataPath;
                    break;
            }
            if (m_leaf.Length > 0)
            {
                ret += "/";
                ret += m_leaf;
            }
            return ret;
        }

        public void CreateDirectory()
        {
            System.IO.Directory.CreateDirectory(GetPath());
        }
    }
}