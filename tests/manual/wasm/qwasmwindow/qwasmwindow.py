# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from selenium.webdriver import Chrome
from selenium.webdriver.common.actions.action_builder import ActionBuilder
from selenium.webdriver.common.actions.pointer_actions import PointerActions
from selenium.webdriver.common.actions.interaction import POINTER_TOUCH
from selenium.webdriver.common.actions.pointer_input import PointerInput
from selenium.webdriver.common.by import By
from selenium.webdriver.support.expected_conditions import presence_of_element_located
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.action_chains import ActionChains

import unittest
from enum import Enum, auto

class WidgetTestCase(unittest.TestCase):
    def setUp(self):
        self._driver = Chrome()
        self._driver.get(
            'http://localhost:8001/qwasmwindow_harness.html')
        self._test_sandbox_element = WebDriverWait(self._driver, 30).until(
            presence_of_element_located((By.ID, 'test-sandbox'))
        )
        self.addTypeEqualityFunc(Rect, assert_rects_equal)

    def test_window_resizing(self):
        defaultWindowMinSize = 100
        screen = Screen(self._driver, ScreenPosition.FIXED,
                       x=0, y=0, width=600, height=600)
        window = Window(screen, x=100, y=100, width=200, height=200)
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
        self.assertEqual(window.rect, Rect(x=75, y=95, width=defaultWindowMinSize, height=defaultWindowMinSize))

    def test_cannot_resize_over_screen_top_edge(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                       x=200, y=200, width=300, height=300)
        window = Window(screen, x=300, y=300, width=100, height=100)
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
        window = Window(screen, x=300, y=300, width=100, height=100)
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
        window = Window(screen, x=300, y=300, width=100, height=100)
        self.assertEqual(window.rect, Rect(x=300, y=300, width=100, height=100))

        window.drag(Handle.TOP_WINDOW_BAR, direction=LEFT(300))
        self.assertEqual(window.frame_rect.x, screen.rect.x - window.frame_rect.width / 2)

    def test_screen_in_scroll_container_limits_window_moves(self):
        screen = Screen(self._driver, ScreenPosition.IN_SCROLL_CONTAINER,
                        x=200, y=2000, width=300, height=300,
                        container_width=500, container_height=7000)
        screen.scroll_to()
        window = Window(screen, x=300, y=2100, width=100, height=100)
        self.assertEqual(window.rect, Rect(x=300, y=2100, width=100, height=100))

        window.drag(Handle.TOP_WINDOW_BAR, direction=LEFT(300))
        self.assertEqual(window.frame_rect.x, screen.rect.x - window.frame_rect.width / 2)

    def test_maximize(self):
        screen = Screen(self._driver, ScreenPosition.RELATIVE,
                        x=200, y=200, width=300, height=300)
        window = Window(screen, x=300, y=300, width=100, height=100, title='Maximize')
        self.assertEqual(window.rect, Rect(x=300, y=300, width=100, height=100))

        window.maximize()
        self.assertEqual(window.frame_rect, Rect(x=200, y=200, width=300, height=300))

    def test_multitouch_window_move(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)
        windows = [Window(screen, x=50, y=50, width=100, height=100, title='First'),
                   Window(screen, x=400, y=400, width=100, height=100, title='Second'),
                   Window(screen, x=50, y=400, width=100, height=100, title='Third')]
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

    def test_multitouch_window_resize(self):
        screen = Screen(self._driver, ScreenPosition.FIXED,
                        x=0, y=0, width=800, height=800)
        windows = [Window(screen, x=50, y=50, width=150, height=150, title='First'),
                   Window(screen, x=400, y=400, width=150, height=150, title='Second'),
                   Window(screen, x=50, y=400, width=150, height=150, title='Third')]
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

    def tearDown(self):
        self._driver.quit()


class ScreenPosition(Enum):
    FIXED = auto()
    RELATIVE = auto()
    IN_SCROLL_CONTAINER = auto()


class Screen:
    def __init__(self, driver, positioning, x, y, width, height, container_width=0, container_height=0):
        self.driver = driver
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        if positioning == ScreenPosition.FIXED:
            command = f'initializeScreenWithFixedPosition({self.x}, {self.y}, {self.width}, {self.height})'
        elif positioning == ScreenPosition.RELATIVE:
            command = f'initializeScreenWithRelativePosition({self.x}, {self.y}, {self.width}, {self.height})'
        elif positioning == ScreenPosition.IN_SCROLL_CONTAINER:
            command = f'initializeScreenInScrollContainer({container_width}, {container_height}, {self.x}, {self.y}, {self.width}, {self.height})'
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

    def scroll_to(self):
        ActionChains(self.driver).scroll_to_element(self.element).perform()


class Window:
    def __init__(self, screen, x, y, width, height, title='title'):
        self.driver = screen.driver
        self.title = title
        self.driver.execute_script(
            f'''
                instance.createWindow({x}, {y}, {width}, {height}, '{screen.screen_info["name"]}', '{title}');
            '''
        )
        self._window_id = self.__window_information()['id']
        self.element = screen.element.shadow_root.find_element(
            By.CSS_SELECTOR, f'#qt-window-{self._window_id}')

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


class Rect:
    def __init__(self, x, y, width, height) -> None:
        self.x = x
        self.y = y
        self.width = width
        self.height = height

    def __str__(self):
        return f'(x: {self.x}, y: {self.y}, width: {self.width}, height: {self.height})'


def assert_rects_equal(geo1, geo2, msg=None):
    if geo1.x != geo2.x or geo1.y != geo2.y or geo1.width != geo2.width or geo1.height != geo2.height:
        raise AssertionError(f'Rectangles not equal: \n{geo1} \nvs \n{geo2}')


unittest.main()
