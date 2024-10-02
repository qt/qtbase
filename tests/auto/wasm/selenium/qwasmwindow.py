# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from selenium.webdriver import Chrome
from selenium.webdriver.chrome.service import Service as ChromeService
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.actions.action_builder import ActionBuilder
from selenium.webdriver.common.actions.pointer_actions import PointerActions
from selenium.webdriver.common.actions.interaction import POINTER_TOUCH
from selenium.webdriver.common.actions.pointer_input import PointerInput
from selenium.webdriver.common.by import By
from selenium.webdriver.support.expected_conditions import presence_of_element_located
from selenium.webdriver.support.ui import WebDriverWait
from webdriver_manager.chrome import ChromeDriverManager

import time
import unittest
from enum import Enum, auto

class WidgetTestCase(unittest.TestCase):
    def setUp(self):
        self._driver = Chrome(service=ChromeService(ChromeDriverManager().install()))
        self._driver.get(
            'http://localhost:8001/tst_qwasmwindow_harness.html')
        self._test_sandbox_element = WebDriverWait(self._driver, 30).until(
            presence_of_element_located((By.ID, 'test-sandbox'))
        )
        self.addTypeEqualityFunc(Color, assert_colors_equal)
        self.addTypeEqualityFunc(Rect, assert_rects_equal)

    def test_hasFocus_returnsFalse_whenSetNoFocusShowWasCalled(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                   x=0, y=0, width=600, height=1200)

        w0 = Widget(self._driver, "w0")
        w0.show()
        w0.showToolTip()

        #
        # Wait for tooltip to disappear
        #
        time.sleep(60)
        #time.sleep(3600)
        self.assertEqual(w0.hasFocus(), True)

        w1 = Widget(self._driver, "w1")
        w1.setNoFocusShow()
        w1.show()
        self.assertEqual(w0.hasFocus(), True)
        self.assertEqual(w1.hasFocus(), False)

        w2 = Widget(self._driver, "w2")
        w2.show()
        self.assertEqual(w0.hasFocus(), False)
        self.assertEqual(w1.hasFocus(), False)
        self.assertEqual(w2.hasFocus(), True)

        w3 = Widget(self._driver,  "w3")
        w3.setNoFocusShow()
        w3.show()
        self.assertEqual(w0.hasFocus(), False)
        self.assertEqual(w1.hasFocus(), False)
        self.assertEqual(w2.hasFocus(), True)
        self.assertEqual(w3.hasFocus(), False)
        w3.activate();
        self.assertEqual(w0.hasFocus(), False)
        self.assertEqual(w1.hasFocus(), False)
        self.assertEqual(w2.hasFocus(), False)
        self.assertEqual(w3.hasFocus(), True)

        clearWidgets(self._driver)

    #Looks weird, no asserts, the test is that
    #the test itself finishes
    def test_showContextMenu_doesNotDeadlock(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                   x=0, y=0, width=600, height=1200)

        w0 = Widget(self._driver, "w0")
        w0.show()
        w0.showToolTip()

        w1 = Widget(self._driver, "w1")
        w1.setNoFocusShow()
        w1.show()
        w1.showToolTip()

        w2 = Widget(self._driver, "w2")
        w2.show()
        w2.showToolTip()

        w3 = Widget(self._driver,  "w3")
        w3.setNoFocusShow()
        w3.show()
        w3.showToolTip()

        w3.activate();
        w3.showToolTip();

        clearWidgets(self._driver)

    def test_window_resizing(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                       x=0, y=0, width=600, height=600)

        window = Window(parent=screen, rect=Rect(x=100, y=100, width=200, height=200))
        self.assertEqual(window.rect, Rect(x=100, y=100, width=200, height=200))

        window.drag(Handle.TOP_LEFT, direction=UP(10) + LEFT(10))
        self.assertEqual(window.rect, Rect(x=90, y=90, width=210, height=210))

        window.drag(Handle.TOP, direction=DOWN(10) + LEFT(100))
        self.assertEqual(window.rect, Rect(x=90, y=100, width=210, height=200))

        window.drag(Handle.TOP_RIGHT, direction=UP(5) + LEFT(5))
        self.assertEqual(window.rect, Rect(x=90, y=95, width=205, height=205))

        window.drag(Handle.RIGHT, direction=DOWN(100) + RIGHT(5))
        self.assertEqual(window.rect, Rect(x=90, y=95, width=210, height=205))

        window.drag(Handle.BOTTOM_RIGHT, direction=UP(5) + LEFT(10))
        self.assertEqual(window.rect, Rect(x=90, y=95, width=200, height=200))

        window.drag(Handle.BOTTOM, direction=DOWN(20) + LEFT(100))
        self.assertEqual(window.rect, Rect(x=90, y=95, width=200, height=220))

        window.drag(Handle.BOTTOM_LEFT, direction=DOWN(10) + LEFT(10))
        self.assertEqual(window.rect, Rect(x=80, y=95, width=210, height=230))

        window.drag(Handle.LEFT, direction=DOWN(343) + LEFT(5))
        self.assertEqual(window.rect, Rect(x=75, y=95, width=215, height=230))

        window.drag(Handle.BOTTOM_RIGHT, direction=UP(150) + LEFT(150))
        self.assertEqual(window.rect, Rect(x=75, y=95, width=65, height=80))

    def test_cannot_resize_over_screen_top_edge(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                       x=200, y=200, width=300, height=300)
        window = Window(parent=screen, rect=Rect(x=300, y=300, width=100, height=100))
        self.assertEqual(window.rect, Rect(x=300, y=300, width=100, height=100))
        frame_rect_before_resize = window.frame_rect

        window.drag(Handle.TOP, direction=UP(200))
        self.assertEqual(window.rect.x, 300)
        self.assertEqual(window.frame_rect.y, screen.rect.y)
        self.assertEqual(window.rect.width, 100)
        self.assertEqual(window.frame_rect.y + window.frame_rect.height,
                         frame_rect_before_resize.y + frame_rect_before_resize.height)

    def test_window_move(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                       x=200, y=200, width=300, height=300)
        window = Window(parent=screen, rect=Rect(x=300, y=300, width=100, height=100))
        self.assertEqual(window.rect, Rect(x=300, y=300, width=100, height=100))

        window.drag(Handle.TOP_WINDOW_BAR, direction=UP(30))
        self.assertEqual(window.rect, Rect(x=300, y=270, width=100, height=100))

        window.drag(Handle.TOP_WINDOW_BAR, direction=RIGHT(50))
        self.assertEqual(window.rect, Rect(x=350, y=270, width=100, height=100))

        window.drag(Handle.TOP_WINDOW_BAR, direction=DOWN(30) + LEFT(70))
        self.assertEqual(window.rect, Rect(x=280, y=300, width=100, height=100))

    def test_screen_limits_window_moves(self):
        screen = Screen(self._driver, ScreenPosition.RELATIVE,
                       x=200, y=200, width=300, height=300)
        window = Window(parent=screen, rect=Rect(x=300, y=300, width=100, height=100))
        self.assertEqual(window.rect, Rect(x=300, y=300, width=100, height=100))

        window.drag(Handle.TOP_WINDOW_BAR, direction=LEFT(300))
        self.assertEqual(window.frame_rect.x, screen.rect.x - window.frame_rect.width / 2)

    def test_screen_in_scroll_container_limits_window_moves(self):
        screen = Screen(self._driver, ScreenPosition.IN_SCROLL_CONTAINER,
                        x=200, y=2000, width=300, height=300,
                        container_width=500, container_height=7000)
        screen.scroll_to()
        window = Window(parent=screen, rect=Rect(x=300, y=2100, width=100, height=100))
        self.assertEqual(window.rect, Rect(x=300, y=2100, width=100, height=100))

        window.drag(Handle.TOP_WINDOW_BAR, direction=LEFT(300))
        self.assertEqual(window.frame_rect.x, screen.rect.x - window.frame_rect.width / 2)

    def test_maximize(self):
        screen = Screen(self._driver, ScreenPosition.RELATIVE,
                        x=200, y=200, width=300, height=300)
        window = Window(parent=screen, rect=Rect(x=300, y=300, width=100, height=100), title='Maximize')
        self.assertEqual(window.rect, Rect(x=300, y=300, width=100, height=100))

        window.maximize()
        self.assertEqual(window.frame_rect, Rect(x=200, y=200, width=300, height=300))

    def test_multitouch_window_move(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)
        windows = [Window(screen, rect=Rect(x=50, y=50, width=100, height=100), title='First'),
                   Window(screen, rect=Rect(x=400, y=400, width=100, height=100), title='Second'),
                   Window(screen, rect=Rect(x=50, y=400, width=100, height=100), title='Third')]

        self.assertEqual(windows[0].rect, Rect(x=50, y=50, width=100, height=100))
        self.assertEqual(windows[1].rect, Rect(x=400, y=400, width=100, height=100))
        self.assertEqual(windows[2].rect, Rect(x=50, y=400, width=100, height=100))

        actions = [TouchDragAction(origin=windows[0].at(Handle.TOP_WINDOW_BAR), direction=DOWN(20) + RIGHT(20)),
                   TouchDragAction(origin=windows[1].at(Handle.TOP_WINDOW_BAR), direction=DOWN(20) + LEFT(20)),
                   TouchDragAction(origin=windows[2].at(Handle.TOP_WINDOW_BAR), direction=UP(20) + RIGHT(20))]
        perform_touch_drag_actions(actions)
        self.assertEqual(windows[0].rect, Rect(x=70, y=70, width=100, height=100))
        self.assertEqual(windows[1].rect, Rect(x=380, y=420, width=100, height=100))
        self.assertEqual(windows[2].rect, Rect(x=70, y=380, width=100, height=100))

    #TODO FIX IN CI
    @unittest.skip('Skip temporarily')
    def test_multitouch_window_resize(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)
        windows = [Window(screen, rect=Rect(x=50, y=50, width=150, height=150), title='First'),
                   Window(screen, rect=Rect(x=400, y=400, width=150, height=150), title='Second'),
                   Window(screen, rect=Rect(x=50, y=400, width=150, height=150), title='Third')]

        self.assertEqual(windows[0].rect, Rect(x=50, y=50, width=150, height=150))
        self.assertEqual(windows[1].rect, Rect(x=400, y=400, width=150, height=150))
        self.assertEqual(windows[2].rect, Rect(x=50, y=400, width=150, height=150))

        actions = [TouchDragAction(origin=windows[0].at(Handle.TOP_LEFT), direction=DOWN(20) + RIGHT(20)),
                   TouchDragAction(origin=windows[1].at(Handle.TOP), direction=DOWN(20) + LEFT(20)),
                   TouchDragAction(origin=windows[2].at(Handle.BOTTOM_RIGHT), direction=UP(20) + RIGHT(20))]
        perform_touch_drag_actions(actions)
        self.assertEqual(windows[0].rect, Rect(x=70, y=70, width=130, height=130))
        self.assertEqual(windows[1].rect, Rect(x=400, y=420, width=150, height=130))
        self.assertEqual(windows[2].rect, Rect(x=50, y=400, width=170, height=130))

    def test_newly_created_window_gets_keyboard_focus(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)
        window = Window(parent=screen, rect=Rect(x=0, y=0, width=800, height=800), title='root')

        ActionChains(self._driver).key_down('c').key_up('c').perform()

        events = window.events
        self.assertEqual(len(events), 2)
        self.assertEqual(events[-2]['type'], 'keyPress')
        self.assertEqual(events[-2]['key'], 'c')
        self.assertEqual(events[-1]['type'], 'keyRelease')
        self.assertEqual(events[-1]['key'], 'c')

    #TODO FIX IN CI
    @unittest.skip('Does not work in CI')
    def test_child_window_activation(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)

        root = Window(parent=screen, rect=Rect(x=0, y=0, width=800, height=800), title='root')
        w1 = Window(parent=root, rect=Rect(x=100, y=100, width=600, height=600), title='w1')
        w1_w1 = Window(parent=w1, rect=Rect(x=100, y=100, width=300, height=300), title='w1_w1')
        w1_w1_w1 = Window(parent=w1_w1, rect=Rect(x=100, y=100, width=100, height=100), title='w1_w1_w1')
        w1_w1_w2 = Window(parent=w1_w1, rect=Rect(x=150, y=150, width=100, height=100), title='w1_w1_w2')
        w1_w2 = Window(parent=w1, rect=Rect(x=300, y=300, width=300, height=300), title='w1_w2')
        w1_w2_w1 = Window(parent=w1_w2, rect=Rect(x=100, y=100, width=100, height=100), title='w1_w2_w1')
        w2 = Window(parent=root, rect=Rect(x=300, y=300, width=450, height=450), title='w2')

        self.assertEqual(screen.window_stack_at_point(*w1_w1.bounding_box.center),
                         [w2, w1_w1_w2, w1_w1_w1, w1_w1, w1, root])

        self.assertEqual(screen.window_stack_at_point(*w2.bounding_box.center),
                         [w2, w1_w2_w1, w1_w2, w1, root])

        for w in [w1, w1_w1, w1_w1_w1, w1_w1_w2, w1_w2, w1_w2_w1]:
            self.assertFalse(w.active)
        self.assertTrue(w2.active)

        w1.click(0, 0)

        for w in [w1, w1_w2, w1_w2_w1]:
            self.assertTrue(w.active)
        for w in [w1_w1, w1_w1_w1, w1_w1_w2, w2]:
            self.assertFalse(w.active)

        self.assertEqual(screen.window_stack_at_point(*w2.bounding_box.center),
                         [w1_w2_w1, w1_w2, w1, w2, root])

        w1_w1_w1.click(0, 0)

        for w in [w1, w1_w1, w1_w1_w1]:
            self.assertTrue(w.active)
        for w in [w1_w1_w2, w1_w2, w1_w2_w1, w2]:
            self.assertFalse(w.active)

        self.assertEqual(screen.window_stack_at_point(*w1_w1_w1.bounding_box.center),
                         [w1_w1_w1, w1_w1_w2, w1_w1, w1, w2, root])

        w1_w1_w2.click(w1_w1_w2.bounding_box.width, w1_w1_w2.bounding_box.height)

        for w in [w1, w1_w1, w1_w1_w2]:
            self.assertTrue(w.active)
        for w in [w1_w1_w1, w1_w2, w1_w2_w1, w2]:
            self.assertFalse(w.active)

        self.assertEqual(screen.window_stack_at_point(w1_w1_w2.bounding_box.x, w1_w1_w2.bounding_box.y),
                         [w1_w1_w2, w1_w1_w1, w1_w1, w1, w2, root])

    def test_window_reparenting(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)

        bottom = Window(parent=screen, rect=Rect(x=800, y=800, width=300, height=300), title='bottom')
        w1 = Window(parent=screen, rect=Rect(x=50, y=50, width=300, height=300), title='w1')
        w2 = Window(parent=screen, rect=Rect(x=50, y=50, width=300, height=300), title='w2')
        w3 = Window(parent=screen, rect=Rect(x=50, y=50, width=300, height=300), title='w3')

        self.assertTrue(
            w2.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w2.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])

        w2.set_parent(w1)

        self.assertTrue(
            w2.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w2.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])

        w3.set_parent(w2)

        self.assertTrue(
            w2.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element in [*w3.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w2.element in [*w3.element.find_elements(By.XPATH, "ancestor::div")])

        w2.set_parent(screen)

        self.assertTrue(
            w2.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w2.element in [*w3.element.find_elements(By.XPATH, "ancestor::div")])

        w1.set_parent(w2)

        self.assertTrue(
            w2.element in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w2.element in [*w3.element.find_elements(By.XPATH, "ancestor::div")])

        w3.set_parent(screen)

        self.assertTrue(
            w2.element in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w2.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])

        w2.set_parent(w3)

        self.assertTrue(
            w2.element in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element in [*w1.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w3.element in [*w2.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w1.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])
        self.assertTrue(
            w2.element not in [*w3.element.find_elements(By.XPATH, "ancestor::div")])

    def test_window_closing(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)

        bottom = Window(parent=screen, rect=Rect(x=800, y=800, width=300, height=300), title='root')
        bottom.close()

        w1 = Window(parent=screen, rect=Rect(x=50, y=50, width=300, height=300), title='w1')
        w2 = Window(parent=screen, rect=Rect(x=50, y=50, width=300, height=300), title='w2')
        w3 = Window(parent=screen, rect=Rect(x=50, y=50, width=300, height=300), title='w3')

        w3.close()

        self.assertFalse(w3 in screen.query_windows())
        self.assertTrue(w2 in screen.query_windows())
        self.assertTrue(w1 in screen.query_windows())

        w4 = Window(parent=screen, rect=Rect(x=50, y=50, width=300, height=300), title='w4')

        self.assertTrue(w4 in screen.query_windows())
        self.assertTrue(w2 in screen.query_windows())
        self.assertTrue(w1 in screen.query_windows())

        w2.close()
        w1.close()

        self.assertTrue(w4 in screen.query_windows())
        self.assertFalse(w2 in screen.query_windows())
        self.assertFalse(w1 in screen.query_windows())

        w4.close()

        self.assertFalse(w4 in screen.query_windows())

    def test_window_painting(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)
        bottom = Window(parent=screen, rect=Rect(x=0, y=0, width=400, height=400), title='root')
        bottom.set_background_color(Color(r=255, g=0, b=0))
        wait_for_animation_frame(self._driver)

        self.assertEqual(bottom.color_at(0, 0), Color(r=255, g=0, b=0))

        w1 = Window(parent=screen, rect=Rect(x=100, y=100, width=600, height=600), title='w1')
        w1.set_background_color(Color(r=0, g=255, b=0))
        wait_for_animation_frame(self._driver)

        self.assertEqual(w1.color_at(0, 0), Color(r=0, g=255, b=0))

        w1_w1 = Window(parent=screen, rect=Rect(x=100, y=100, width=400, height=400), title='w1_w1')
        w1_w1.set_parent(w1)
        w1_w1.set_background_color(Color(r=0, g=0, b=255))
        wait_for_animation_frame(self._driver)

        self.assertEqual(w1_w1.color_at(0, 0), Color(r=0, g=0, b=255))

        w1_w1_w1 = Window(parent=screen, rect=Rect(x=100, y=100, width=200, height=200), title='w1_w1_w1')
        w1_w1_w1.set_parent(w1_w1)
        w1_w1_w1.set_background_color(Color(r=255, g=255, b=0))
        wait_for_animation_frame(self._driver)

        self.assertEqual(w1_w1_w1.color_at(0, 0), Color(r=255, g=255, b=0))

    def test_opengl_painting(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)
        bottom = Window(parent=screen, rect=Rect(x=0, y=0, width=400, height=400), title='root',opengl=1)
        bottom.set_background_color(Color(r=255, g=0, b=0))
        wait_for_animation_frame(self._driver)
        time.sleep(1)

        self.assertEqual(bottom.window_color_at_0_0(), Color(r=255, g=0, b=0))

        w1 = Window(parent=screen, rect=Rect(x=100, y=100, width=600, height=600), title='w1', opengl=1)
        w1.set_background_color(Color(r=0, g=255, b=0))
        wait_for_animation_frame(self._driver)
        time.sleep(1)

        self.assertEqual(w1.window_color_at_0_0(), Color(r=0, g=255, b=0))

        w1_w1 = Window(parent=screen, rect=Rect(x=100, y=100, width=400, height=400), title='w1_w1', opengl=1)
        w1_w1.set_parent(w1)
        w1_w1.set_background_color(Color(r=0, g=0, b=255))
        wait_for_animation_frame(self._driver)
        time.sleep(1)

        self.assertEqual(w1_w1.window_color_at_0_0(), Color(r=0, g=0, b=255))

        w1_w1_w1 = Window(parent=screen, rect=Rect(x=100, y=100, width=200, height=200), title='w1_w1_w1', opengl=1)
        w1_w1_w1.set_parent(w1_w1)
        w1_w1_w1.set_background_color(Color(r=255, g=255, b=0))
        wait_for_animation_frame(self._driver)
        time.sleep(1)

        self.assertEqual(w1_w1_w1.window_color_at_0_0(), Color(r=255, g=255, b=0))

#TODO FIX IN CI
    @unittest.skip('Does not work in CI')
    def test_keyboard_input(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)

        bottom = Window(parent=screen, rect=Rect(x=0, y=0, width=800, height=800), title='root')
        w1 = Window(parent=screen, rect=Rect(x=100, y=100, width=600, height=600), title='w1')
        w1_w1 = Window(parent=w1, rect=Rect(x=100, y=100, width=400, height=400), title='w1_w1')
        w1_w1_w1 = Window(parent=w1_w1, rect=Rect(x=100, y=100, width=100, height=100), title='w1_w1_w1')
        Window(parent=w1_w1, rect=Rect(x=150, y=150, width=100, height=100), title='w1_w1_w2')

        w1_w1_w1.click(0, 0)

        ActionChains(self._driver).key_down('c').key_up('c').perform()

        events = w1_w1_w1.events
        self.assertEqual(len(events), 2)
        self.assertEqual(events[-2]['type'], 'keyPress')
        self.assertEqual(events[-2]['key'], 'c')
        self.assertEqual(events[-1]['type'], 'keyRelease')
        self.assertEqual(events[-1]['key'], 'c')
        self.assertEqual(len(w1_w1.events), 0)
        self.assertEqual(len(w1.events), 0)

        w1_w1.click(0, 0)

        ActionChains(self._driver).key_down('b').key_up('b').perform()

        events = w1_w1.events
        self.assertEqual(len(events), 2)
        self.assertEqual(events[-2]['type'], 'keyPress')
        self.assertEqual(events[-2]['key'], 'b')
        self.assertEqual(events[-1]['type'], 'keyRelease')
        self.assertEqual(events[-1]['key'], 'b')
        self.assertEqual(len(w1_w1_w1.events), 2)
        self.assertEqual(len(w1.events), 0)

        w1.click(0, 0)

        ActionChains(self._driver).key_down('a').key_up('a').perform()

        events = w1.events
        self.assertEqual(len(events), 2)
        self.assertEqual(events[-2]['type'], 'keyPress')
        self.assertEqual(events[-2]['key'], 'a')
        self.assertEqual(events[-1]['type'], 'keyRelease')
        self.assertEqual(events[-1]['key'], 'a')
        self.assertEqual(len(w1_w1_w1.events), 2)
        self.assertEqual(len(w1_w1.events), 2)

    def tearDown(self):
        self._driver.quit()

class ScreenPosition(Enum):
    FIXED = auto()
    RELATIVE = auto()
    IN_SCROLL_CONTAINER = auto()

class Screen:
    def __init__(self, driver, positioning=None, x=None, y=None, width=None, height=None, container_width=0, container_height=0, screen_name=None):
        self.driver = driver
        if screen_name is not None:
            screen_information = call_instance_function(self.driver, 'screenInformation')
            if len(screen_information) != 1:
                raise AssertionError('Expecting exactly one screen_information!')
            self.screen_info = screen_information[0]
            self.element = driver.find_element(By.CSS_SELECTOR, f'#test-screen-1')
            return

        if positioning == ScreenPosition.FIXED:
            command = f'initializeScreenWithFixedPosition({x}, {y}, {width}, {height})'
        elif positioning == ScreenPosition.RELATIVE:
            command = f'initializeScreenWithRelativePosition({x}, {y}, {width}, {height})'
        elif positioning == ScreenPosition.IN_SCROLL_CONTAINER:
            command = f'initializeScreenInScrollContainer({container_width}, {container_height}, {x}, {y}, {width}, {height})'
        self.element = self.driver.execute_script(
            f'''
                return testSupport.{command};
            '''
        )
        if positioning == ScreenPosition.IN_SCROLL_CONTAINER:
            self.element = self.element[1]

        screen_information = call_instance_function(
            self.driver, 'screenInformation')
        if len(screen_information) != 1:
            raise AssertionError('Expecting exactly one screen_information!')
        self.screen_info = screen_information[0]

    @property
    def rect(self):
        self.screen_info = call_instance_function(
            self.driver, 'screenInformation')[0]
        geo = self.screen_info['geometry']
        return Rect(geo['x'], geo['y'], geo['width'], geo['height'])

    @property
    def name(self):
        return self.screen_info['name']

    def scroll_to(self):
        ActionChains(self.driver).scroll_to_element(self.element).perform()

    def hit_test_point(self, x, y):
        return self.driver.execute_script(
            f'''
                return testSupport.hitTestPoint({x}, {y}, '{self.element.get_attribute("id")}');
            '''
        )

    def window_stack_at_point(self, x, y):
        return [
            Window(self, element=element) for element in [
                *filter(lambda elem: (elem.get_attribute('id') if elem.get_attribute('id') is not None else '')
                        .startswith('qt-window-'), self.hit_test_point(x, y))]]

    def query_windows(self):
        shadow_container = self.element.find_element(By.CSS_SELECTOR, f'#qt-shadow-container')
        return [
            Window(self, element=element) for element in shadow_container.shadow_root.find_elements(
                    By.CSS_SELECTOR, f'div#{self.name} > div.qt-window')]

    def find_element(self, method, query):
        shadow_container = self.element.find_element(By.CSS_SELECTOR, f'#qt-shadow-container')
        return shadow_container.shadow_root.find_element(method, query)

def clearWidgets(driver):
    driver.execute_script(
            f'''
                instance.clearWidgets();
            '''
        )

class Widget:
    def __init__(self, driver, name):
        self.name=name
        self.driver=driver

        self.driver.execute_script(
            f'''
                instance.createWidget('{self.name}');
            '''
        )

    def setNoFocusShow(self):
        self.driver.execute_script(
            f'''
                instance.setWidgetNoFocusShow('{self.name}');
            '''
        )

    def show(self):
        self.driver.execute_script(
            f'''
                instance.showWidget('{self.name}');
            '''
        )
    def hasFocus(self):
        focus = call_instance_function_arg(self.driver, 'hasWidgetFocus', self.name)
        return focus

    def activate(self):
        self.driver.execute_script(
            f'''
                instance.activateWidget('{self.name}');
            '''
        )

    def showContextMenu(self):
        self.driver.execute_script(
            f'''
                instance.showContextMenuWidget('{self.name}');
            '''
        )

    def showToolTip(self):
        self.driver.execute_script(
            f'''
                instance.showToolTipWidget('{self.name}');
            '''
        )

class Window:
    def __init__(self, parent=None, rect=None, title=None, element=None, visible=True, opengl=0):
        self.driver = parent.driver
        self.opengl = opengl
        if element is not None:
            self.element = element
            self.title = element.find_element(
                    By.CSS_SELECTOR, f'.title-bar > .window-name').get_property("textContent")
            information = self.__window_information()
            self.screen = Screen(self.driver, screen_name=information['screen']['name'])
            pass
        else:
            self.title = title = title if title is not None else 'window'
            if isinstance(parent, Window):
                self.driver.execute_script(
                    f'''
                        instance.createWindow({rect.x}, {rect.y}, {rect.width}, {rect.height}, 'window', '{parent.title}', '{title}', {opengl});
                    '''
                )
                self.screen = parent.screen
            else:
                assert(isinstance(parent, Screen))
                self.driver.execute_script(
                    f'''
                        instance.createWindow({rect.x}, {rect.y}, {rect.width}, {rect.height}, 'screen', '{parent.name}', '{title}', {opengl});
                    '''
                )
                self.screen = parent
        self._window_id = self.__window_information()['id']
        self.element = self.screen.find_element(
                By.CSS_SELECTOR, f'#qt-window-{self._window_id}')
        if visible:
            self.set_visible(True)

    def __eq__(self, other):
        return self._window_id == other._window_id if isinstance(other, Window) else False

    def __window_information(self):
        information = call_instance_function(self.driver, 'windowInformation')
        return next(filter(lambda e: e['title'] == self.title, information))

    @property
    def rect(self):
        geo = self.__window_information()["geometry"]
        return Rect(geo['x'], geo['y'], geo['width'], geo['height'])

    @property
    def frame_rect(self):
        geo = self.__window_information()["frameGeometry"]
        return Rect(geo['x'], geo['y'], geo['width'], geo['height'])

    @property
    def events(self):
        events = self.driver.execute_script(
            f'''
                return testSupport.events();
            '''
        )
        return [*filter(lambda e: e['windowTitle'] == self.title, events)]

    def set_visible(self, visible):
        info = self.__window_information()
        self.driver.execute_script(
            f'''instance.setWindowVisible({info['id']}, {'true' if visible else 'false'});''')

    def drag(self, handle, direction):
        ActionChains(self.driver)                                                        \
            .move_to_element_with_offset(self.element, *self.at(handle)['offset'])       \
            .click_and_hold()                                                            \
            .move_by_offset(*translate_direction_to_offset(direction))                   \
            .release().perform()

    def maximize(self):
        maximize_button = self.element.find_element(
            By.CSS_SELECTOR, f'.title-bar :nth-child(6)')
        maximize_button.click()

    def at(self, handle):
        """ Returns (window, offset) for given handle on window"""
        width = self.frame_rect.width
        height = self.frame_rect.height

        if handle == Handle.TOP_LEFT:
            offset = (-width/2, -height/2)
        elif handle == Handle.TOP:
            offset = (0, -height/2)
        elif handle == Handle.TOP_RIGHT:
            offset = (width/2, -height/2)
        elif handle == Handle.LEFT:
            offset = (-width/2, 0)
        elif handle == Handle.RIGHT:
            offset = (width/2, 0)
        elif handle == Handle.BOTTOM_LEFT:
            offset = (-width/2, height/2)
        elif handle == Handle.BOTTOM:
            offset = (0, height/2)
        elif handle == Handle.BOTTOM_RIGHT:
            offset = (width/2, height/2)
        elif handle == Handle.TOP_WINDOW_BAR:
            frame_top = self.frame_rect.y
            client_area_top = self.rect.y
            top_frame_bar_width = client_area_top - frame_top
            offset = (0, -height/2 + top_frame_bar_width/2)
        return {'window': self, 'offset': offset}

    @property
    def bounding_box(self):
        raw = self.driver.execute_script("""
            return arguments[0].getBoundingClientRect();
            """, self.element)
        return Rect(raw['x'], raw['y'], raw['width'], raw['height'])

    @property
    def active(self):
        return not self.inactive
        # self.assertFalse('inactive' in window_element.get_attribute(
        #     'class').split(' '), window_element.get_attribute('id'))

    @property
    def inactive(self):
        window_chain = [
            *self.element.find_elements(By.XPATH, "ancestor::div"), self.element]
        return next(filter(lambda elem: 'qt-window' in elem.get_attribute('class').split(' ') and
                           'inactive' in elem.get_attribute(
                               'class').split(' '),
                           window_chain
                           ), None) is not None

    def click(self, x, y):
        rect = self.bounding_box

        SELENIUM_IMPRECISION_COMPENSATION = 2
        ActionChains(self.driver).move_to_element(
            self.element).move_by_offset(-rect.width / 2 + x + SELENIUM_IMPRECISION_COMPENSATION,
                                         -rect.height / 2 + y + SELENIUM_IMPRECISION_COMPENSATION).click().perform()

    def set_parent(self, parent):
        if isinstance(parent, Screen):
            # TODO won't work with screen that is not parent.screen
            self.screen = parent
            self.driver.execute_script(
                f'''
                    instance.setWindowParent('{self.title}', 'none');
                '''
            )
        else:
            assert(isinstance(parent, Window))
            self.screen = parent.screen
            self.driver.execute_script(
                f'''
                    instance.setWindowParent('{self.title}', '{parent.title}');
                '''
            )

    def close(self):
        self.driver.execute_script(
            f'''
                instance.closeWindow('{self.title}');
            '''
        )

    def window_color_at_0_0(self):
        color = call_instance_function_arg(self.driver, 'getOpenGLColorAt_0_0', self.title)

        wcol = color[0]
        r = wcol['r']
        g = wcol['g']
        b = wcol['b']

        return Color(r,g,b)

    def color_at(self, x, y):
        raw = self.driver.execute_script(
            f'''
                return arguments[0].querySelector('canvas')
                    .getContext('2d').getImageData({x}, {y}, 1, 1).data;
            ''', self.element)
        return Color(r=raw[0], g=raw[1], b=raw[2])

    def set_background_color(self, color):
        return self.driver.execute_script(
            f'''
                return instance.setWindowBackgroundColor('{self.title}', {color.r}, {color.g}, {color.b});
            '''
        )


class TouchDragAction:
    def __init__(self, origin, direction):
        self.origin = origin
        self.direction = direction
        self.step = 2


def perform_touch_drag_actions(actions):
    driver = actions[0].origin['window'].driver
    touch_action_builder = ActionBuilder(driver)
    pointers = [PointerActions(source=touch_action_builder.add_pointer_input(
        POINTER_TOUCH, f'touch_input_{i}')) for i in range(len(actions))]

    for action, pointer in zip(actions, pointers):
        pointer.move_to(
            action.origin['window'].element, *action.origin['offset'])
        pointer.pointer_down(width=10, height=10, pressure=1)
    moves = [translate_direction_to_offset(a.direction) for a in actions]

    def movement_finished():
        for move in moves:
            if move != (0, 0):
                return False
        return True

    def sign(num):
        if num > 0:
            return 1
        elif num < 0:
            return -1
        return 0

    while not movement_finished():
        for i in range(len(actions)):
            pointer = pointers[i]
            move = moves[i]
            step = actions[i].step

            current_move = (
                min(abs(move[0]), step) * sign(move[0]), min(abs(move[1]), step) * sign(move[1]))
            moves[i] = (move[0] - current_move[0], move[1] - current_move[1])
            pointer.move_by(current_move[0],
                            current_move[1], width=10, height=10)
    for pointer in pointers:
        pointer.pointer_up()

    touch_action_builder.perform()


class TouchDragAction:
    def __init__(self, origin, direction):
        self.origin = origin
        self.direction = direction
        self.step = 2


def perform_touch_drag_actions(actions):
    driver = actions[0].origin['window'].driver
    touch_action_builder = ActionBuilder(driver)
    pointers = [PointerActions(source=touch_action_builder.add_pointer_input(
        POINTER_TOUCH, f'touch_input_{i}')) for i in range(len(actions))]

    for action, pointer in zip(actions, pointers):
        pointer.move_to(
            action.origin['window'].element, *action.origin['offset'])
        pointer.pointer_down(width=10, height=10, pressure=1)

    moves = [translate_direction_to_offset(a.direction) for a in actions]

    def movement_finished():
        for move in moves:
            if move != (0, 0):
                return False
        return True

    def sign(num):
        if num > 0:
            return 1
        elif num < 0:
            return -1
        return 0

    while not movement_finished():
        for i in range(len(actions)):
            pointer = pointers[i]
            move = moves[i]
            step = actions[i].step

            current_move = (
                min(abs(move[0]), step) * sign(move[0]), min(abs(move[1]), step) * sign(move[1]))
            moves[i] = (move[0] - current_move[0], move[1] - current_move[1])
            pointer.move_by(current_move[0],
                            current_move[1], width=10, height=10)

    for pointer in pointers:
        pointer.pointer_up()

    touch_action_builder.perform()


def translate_direction_to_offset(direction):
    return (direction.val[1] - direction.val[3], direction.val[2] - direction.val[0])


def call_instance_function(driver, name):
    return driver.execute_script(
        f'''let result;
            window.{name}Callback = data => result = data;
            instance.{name}();
            return eval(result);''')

def call_instance_function_arg(driver, name, arg):
    return driver.execute_script(
        f'''let result;
            window.{name}Callback = data => result = data;
            instance.{name}('{arg}');
            return eval(result);''')

def wait_for_animation_frame(driver):
    driver.execute_script(
        '''
            window.requestAnimationFrame(() => {
                const sync = document.createElement('div');
                sync.id = 'test-sync';
                document.body.appendChild(sync);
            });
        '''
    )
    WebDriverWait(driver, 1).until(
        presence_of_element_located((By.ID, 'test-sync'))
    )
    driver.execute_script(
        '''
            document.body.removeChild(document.body.querySelector('#test-sync'));
        '''
    )

class Direction:
    def __init__(self):
        self.val = (0, 0, 0, 0)

    def __init__(self, north, east, south, west):
        self.val = (north, east, south, west)

    def __add__(self, other):
        return Direction(self.val[0] + other.val[0],
                         self.val[1] + other.val[1],
                         self.val[2] + other.val[2],
                         self.val[3] + other.val[3])


class UP(Direction):
    def __init__(self, step=1):
        self.val = (step, 0, 0, 0)


class RIGHT(Direction):
    def __init__(self, step=1):
        self.val = (0, step, 0, 0)


class DOWN(Direction):
    def __init__(self, step=1):
        self.val = (0, 0, step, 0)


class LEFT(Direction):
    def __init__(self, step=1):
        self.val = (0, 0, 0, step)


class Handle(Enum):
    TOP_LEFT = auto()
    TOP = auto()
    TOP_RIGHT = auto()
    LEFT = auto()
    RIGHT = auto()
    BOTTOM_LEFT = auto()
    BOTTOM = auto()
    BOTTOM_RIGHT = auto()
    TOP_WINDOW_BAR = auto()

class Color:
    def __init__(self, r, g, b):
        self.r = r
        self.g = g
        self.b = b

class Rect:
    def __init__(self, x, y, width, height) -> None:
        self.x = x
        self.y = y
        self.width = width
        self.height = height

    def __str__(self):
        return f'(x: {self.x}, y: {self.y}, width: {self.width}, height: {self.height})'

    @property
    def center(self):
        return self.x + self.width / 2, self.y + self.height / 2,

def assert_colors_equal(color1, color2, msg=None):
    if color1.r != color2.r or color1.g != color2.g or color1.b != color2.b:
        raise AssertionError(f'Colors not equal: \n{color1} \nvs \n{color2}')

def assert_rects_equal(geo1, geo2, msg=None):
    if geo1.x != geo2.x or geo1.y != geo2.y or geo1.width != geo2.width or geo1.height != geo2.height:
        raise AssertionError(f'Rectangles not equal: \n{geo1} \nvs \n{geo2}')

unittest.main()
