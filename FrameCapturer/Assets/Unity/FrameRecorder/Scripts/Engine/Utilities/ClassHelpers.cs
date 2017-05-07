using System;
using System.Collections.Generic;
using System.Reflection;

namespace UnityEngine.Recorder.FrameRecorder.Utilities
{
    public class ClassHelpers
    {
        public static IEnumerable<KeyValuePair<Type, object[]>> FilterByAttribute<T>(bool inherit = false)
        {
            var attribType = typeof(T);
            foreach (var a in AppDomain.CurrentDomain.GetAssemblies())
            {
                foreach (var t in a.GetTypes())
                {
                    var attributes = t.GetCustomAttributes(attribType, inherit);
                    if (attributes.Length != 0)
                        yield return new KeyValuePair<Type, object[]>(t, attributes);
                }
            }
        }

        public static T GetAttribute<T>(Type type) where T : class
        {
            var attributes  = type.GetCustomAttributes(typeof(T), true);
            if (attributes.Length == 0)
                return null;
            else
                return attributes[0] as T;
        }

        public static T GetAttribute<T>(MethodInfo methofInfo) where T : class
        {
            var attributes = methofInfo.GetCustomAttributes(typeof(T), true);

            if (attributes.Length == 0)
                return null;
            else
                return attributes[0] as T;
        }
    }
}
