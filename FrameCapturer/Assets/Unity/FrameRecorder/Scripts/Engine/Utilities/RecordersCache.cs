using System;
using System.Collections.Generic;
using System.Linq;

namespace UnityEngine.Recorder.FrameRecorder.Utilities
{
    public static class RecordersCache
    {
        class CachedRecorder
        {
            public Type type { get; set; }
            public string category { get; set; }
        }

        static List<RecorderInfo> m_RecordersCache;

        static void Init()
        {
            if (m_RecordersCache == null)
            {
                m_RecordersCache = RecordersInventory.recorders
                    .OrderBy((x) => x.Key)
                    .Select((x) => x.Value)
                    .ToList();
            }
        }

        public static int GroupedIndexOfRecorder(Type recorder, string category)
        {
            Init();

            var skipped = 0;
            var any = false;
            for (var i = 0; i < m_RecordersCache.Count; i++)
            {
                if (string.Compare(category, m_RecordersCache[i].category, StringComparison.InvariantCultureIgnoreCase) == 0)
                    skipped++;
                else if (m_RecordersCache[i].recorder == recorder)
                    return i - skipped;
                else
                    any = true;
            }

            if (any)
                return 0;
            else
                return -1;
        }

        public static Type RecorderFromGroupedIndex(int index, string category)
        {
            Init();

            var filteredIndex = 0;
            foreach (var t in m_RecordersCache)
            {
                if (string.Compare(category, t.category, StringComparison.InvariantCultureIgnoreCase) != 0)
                    continue;

                if (index == filteredIndex)
                    return t.recorder;
                filteredIndex++;
            }

            return null;
        }

        public static string[] GetNameOfRecordersInGroup(string category)
        {
            Init();

            // count them
            int count = 0;
            foreach (var item in  m_RecordersCache)
                if (string.Compare(category, item.category, StringComparison.InvariantCultureIgnoreCase) == 0)
                    count++;

            // store them
            var result = new string[count];
            if (count > 0)
            {
                count = 0;
                foreach (var item in  m_RecordersCache)
                    if (string.Compare(category, item.category, StringComparison.InvariantCultureIgnoreCase) == 0)
                        result[count++] = item.recorder.FullName;
            }

            return result;
        }
    }
}
