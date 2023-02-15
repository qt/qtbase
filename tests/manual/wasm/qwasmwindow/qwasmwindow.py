# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from selenium.webdriver import Chrome
from selenium.webdriver.common.by import By
from selenium.webdriver.support.expected_conditions import presence_of_element_located
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.action_chains import ActionChains

import unittest


class WidgetTestCase(unittest.TestCase):
    def setUp(self):
        self._driver = Chrome()
        self._driver.get(
            'http://localhost:8001/qwasmwindow_harness.html')
        self._test_sandbox_element = WebDriverWait(self._driver, 30).until(
            presence_of_element_located((By.ID, 'test-sandbox'))
        )

    def _make_geometry(self, x, y, width, height):
        return {'x': x, 'y': y, 'width': width, 'height': height}

    def test_window_resizing(self):
        screen = self._create_screen_with_fixed_position(0, 0, 600, 600)
        screen_information = self._screen_information()
        self.assertEqual(len(screen_information), 1)
        screen_information = screen_information[-1]
        window = self._create_window(
            100, 100, 200, 200, screen, screen_information["name"], 'title')

        window_information = self._window_information()[0]
        self.assertEqual(
            window_information["geometry"], self._make_geometry(100, 100, 200, 200))

        self._drag_window(window, window_information, [0, 0], [-10, -10])
        window_information = self._window_information()[0]
        self.assertEqual(
            window_information["geometry"], self._make_geometry(90, 90, 210, 210))

        self._drag_window(window, window_information,
                          [window_information['frameGeometry']['width'] / 2, 0], [-100, 10])
        window_information = self._window_information()[0]
        self.assertEqual(
            window_information["geometry"], self._make_geometry(90, 100, 210, 200))

        self._drag_window(window, window_information,
                          [window_information['frameGeometry']['width'], 0], [-5, -5])
        window_information = self._window_information()[0]

        self.assertEqual(
            window_information["geometry"], self._make_geometry(90, 95, 205, 205))

        self._drag_window(window, window_information,
                          [window_information['frameGeometry']['width'],
                           window_information['frameGeometry']['height'] / 2], [5, 100])
        window_information = self._window_information()[0]

        self.assertEqual(
            window_information["geometry"], self._make_geometry(90, 95, 210, 205))

        self._drag_window(window, window_information,
                          [window_information['frameGeometry']['width'],
                           window_information['frameGeometry']['height']], [-10, -5])
        window_information = self._window_information()[0]

        self.assertEqual(self._window_information()[0]["geometry"],
                         self._make_geometry(90, 95, 200, 200))

        self._drag_window(window, window_information, [
            window_information['frameGeometry']['width'] / 2,
            window_information['frameGeometry']['height']], [-100, 20])
        window_information = self._window_information()[0]

        self.assertEqual(self._window_information()[0]["geometry"],
                         self._make_geometry(90, 95, 200, 220))

        self._drag_window(window, window_information, [
            0, window_information['frameGeometry']['height']], [-10, 10])
        window_information = self._window_information()[0]

        self.assertEqual(self._window_information()[0]["geometry"],
                         self._make_geometry(80, 95, 210, 230))

        self._drag_window(window, window_information, [
            0, window_information['frameGeometry']['height'] / 2], [-5, 343])
        window_information = self._window_information()[0]

        self.assertEqual(self._window_information()[0]["geometry"],
                         self._make_geometry(75, 95, 215, 230))

    def test_cannot_resize_over_screen_top_edge(self):
        screen = self._create_screen_with_fixed_position(200, 200, 300, 300)
        screen_information = self._screen_information()[-1]

        window = self._create_window(
            300, 300, 100, 100, screen, screen_information['name'], 'title')

        window_information = self._window_information()[0]
        frame_geometry_before_resize = window_information["frameGeometry"]
        self.assertEqual(
            window_information["geometry"], self._make_geometry(300, 300, 100, 100))

        self._drag_window(window, window_information,
                          [window_information['frameGeometry']['width'] / 2, 0], [0, -200])

        geometry = self._window_information()[0]["geometry"]
        frame_geometry = self._window_information()[0]["frameGeometry"]
        self.assertEqual(geometry['x'], 300)
        self.assertEqual(frame_geometry['y'],
                         screen_information['geometry']['y'])
        self.assertEqual(geometry['width'], 100)
        self.assertEqual(frame_geometry['y'] + frame_geometry['height'],
                         frame_geometry_before_resize['y'] + frame_geometry_before_resize['height'])

    def test_window_move(self):
        screen = self._create_screen_with_fixed_position(200, 200, 300, 300)
        screen_information = self._screen_information()[-1]

        window = self._create_window(
            300, 300, 100, 100, screen, screen_information['name'], 'title')

        self.assertEqual(
            self._window_information()[0]["geometry"], self._make_geometry(300, 300, 100, 100))

        window_information = self._window_information()[0]
        self._drag_window(window, window_information,
                          [window_information['frameGeometry']['width'] / 2, 6], [0, -30])

        self.assertEqual(self._window_information()[0]["geometry"],
                         self._make_geometry(300, 270, 100, 100))

        window_information = self._window_information()[0]
        window_frame_top = window_information['frameGeometry']['y']
        window_client_area_top = window_information['geometry']['y']

        top_frame_height = window_client_area_top - window_frame_top + 1

        self._drag_window(window, window_information,
                          [window_information['frameGeometry']['width'] / 2,
                           top_frame_height], [0, -30])

        self.assertEqual(self._window_information()[0]["geometry"],
                         self._make_geometry(300, 270, 100, 100))

    def test_screen_limits_window_moves(self):
        screen = self._create_screen_with_relative_position(200, 200, 300, 300)
        screen_information = self._screen_information()[-1]

        window = self._create_window(
            300, 300, 100, 100, screen, screen_information['name'], 'title')

        self.assertEqual(self._window_information()[0]["geometry"],
                         self._make_geometry(300, 300, 100, 100))

        window_information = self._window_information()[0]
        self._drag_window(window, window_information,
                          [window_information['frameGeometry']['width'] / 2, 6], [-300, 0])
        self.assertEqual(
            self._window_information()[0]["frameGeometry"]['x'],
            screen_information['geometry']['x'] - window_information['frameGeometry']['width'] / 2)

    def test_screen_in_scroll_container_limits_window_moves(self):
        _, screen = self._create_screen_in_scroll_container(
            500, 7000, 200, 2000, 300, 300)
        screen_information = self._screen_information()[-1]

        ActionChains(self._driver).scroll_to_element(screen).perform()

        window = self._create_window(
            300, 2100, 100, 100, screen, screen_information['name'], 'title')

        self.assertEqual(self._window_information()[0]["geometry"],
                         self._make_geometry(300, 2100, 100, 100))

        window_information = self._window_information()[0]
        self._drag_window(window, window_information,
                          [window_information['frameGeometry']['width'] / 2, 6], [-300, 0])
        self.assertEqual(
            self._window_information()[0]["frameGeometry"]['x'],
            screen_information['geometry']['x'] - window_information['frameGeometry']['width'] / 2)

    def test_maximize(self):
        screen = self._create_screen_with_relative_position(200, 200, 300, 300)
        screen_information = self._screen_information()[-1]

        window = self._create_window(
            300, 300, 100, 100, screen, screen_information['name'], 'Maximize')

        self.assertEqual(self._window_information()[0]["geometry"],
                         self._make_geometry(300, 300, 100, 100))

        maximize_button = window.find_element(
            By.CSS_SELECTOR, f'.title-bar :nth-child(6)')
        maximize_button.click()

        self.assertEqual(
            self._window_information()[0]["frameGeometry"],
            self._make_geometry(200, 200, 300, 300))

    def tearDown(self):
        self._driver.quit()

    def _drag_window(self, window_element, window_information, origin, offset):
        ActionChains(self._driver).move_to_element(
            window_element).move_by_offset(
                -window_information['frameGeometry']['width'] / 2 + origin[0],
                -window_information['frameGeometry']['height'] / 2 + origin[1]).click_and_hold(
        ).move_by_offset(*offset).release().perform()

    def _call_instance_function(self, name):
        return self._driver.execute_script(
            f'''let result;
                window.{name}Callback = data => result = data;
                instance.{name}();
                return eval(result);''')

    def _window_information(self):
        return self._call_instance_function('windowInformation')

    def _screen_information(self):
        return self._call_instance_function('screenInformation')

    def _get_resize_location(self, screen, window_info, which):
        frame_geometry = window_info['frameGeometry']
        frame_x_in_screen = frame_geometry['x'] - screen['geometry']['x']
        frame_y_in_screen = frame_geometry['y'] - screen['geometry']['y']

        return [frame_x_in_screen, frame_y_in_screen]

    def _get_title_bar(self, window):
        return window.find_element(
            By.CSS_SELECTOR, f'.window-name')

    def _create_screen_with_fixed_position(self, x, y, w, h):
        return self._driver.execute_script(
            f'''
                return testSupport.initializeScreenWithFixedPosition({x}, {y}, {w}, {h});
            '''
        )

    def _create_screen_with_relative_position(self, x, y, w, h):
        return self._driver.execute_script(
            f'''
                return testSupport.initializeScreenWithRelativePosition({x}, {y}, {w}, {h});
            '''
        )

    def _create_screen_in_scroll_container(self, containerW, containerH, x, y, w, h):
        return self._driver.execute_script(
            f'''
                return testSupport.initializeScreenInScrollContainer(
                    {containerW}, {containerH}, {x}, {y}, {w}, {h});
            '''
        )

    def _create_window(self, x, y, w, h, screen, screen_id, title):
        self._driver.execute_script(
            f'''
                instance.createWindow({x}, {y}, {w}, {h}, '{screen_id}', '{title}');
            '''
        )
        window_id = self._window_information()[-1]["id"]
        return screen.shadow_root.find_element(By.CSS_SELECTOR, f'#qt-window-{window_id}')

unittest.main()
