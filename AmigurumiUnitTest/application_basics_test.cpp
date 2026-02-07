#include "CppUnitTest.h"
#include "imgui.h"

import application_basics;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace AmigurumiUnitTest
{
	TEST_CLASS(application_basics_test)
	{
	public:
		
		TEST_METHOD(test_imgui_flags)
		{
			ImGuiConfigFlags flags = 0;
			set_imgui_flags(flags);
			Assert::IsTrue( (flags & ImGuiConfigFlags_NavEnableKeyboard)  == ImGuiConfigFlags_NavEnableKeyboard, L"flag keyboard enabled not present");
			Assert::IsTrue((flags & ImGuiConfigFlags_NavEnableGamepad) == ImGuiConfigFlags_NavEnableGamepad, L"flag ghamepad not enabled");
		}

		TEST_METHOD(test_shall_resize)
		{
			Assert::IsFalse(shall_resize(0, 0), L"no resize needed");
			Assert::IsFalse(shall_resize(0, 5), L"resize in height only -- inconsistent");
			Assert::IsFalse(shall_resize(5, 0), L"resize in width only -- inconsistent");
			Assert::IsTrue(shall_resize(5, 5), L"resize in both directions -- correct");
		}
	};
}
