#pragma once

namespace Trinity
{
    namespace Utilities
    {
        class Time
        {
        public:
            // Call once during engine startup
            static void Initialize();
            static void Update();

            static double Now();
            static float DeltaTime();

        private:
            static bool s_Initialized;
            static float s_DeltaTime;
        };
    }
}