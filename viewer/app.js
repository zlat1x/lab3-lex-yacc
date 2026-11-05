
'use strict';

const NS = 'http://www.w3.org/2000/svg';
const graph = document.getElementById('graph');
const scene = document.getElementById('scene');
const canvas = document.getElementById('canvas');
const fileInput = document.getElementById('fileInput');
const status = document.getElementById('status');
const filename = document.getElementById('filename');
const nodeCount = document.getElementById('nodeCount');
const detailsLabel = document.getElementById('detailsLabel');
const detailsType = document.getElementById('detailsType');

let tree = null;
let layout = null;
let collapsed = new WeakSet();
let view = {x: 0, y: 0, w: 1000, h: 600};
let drag = null;

function svgElement(tag, attrs = {}, text) {
    const element = document.createElementNS(NS, tag);

    for (const [key, value] of Object.entries(attrs)) {
        element.setAttribute(key, value);
    }

    if (text !== undefined) {
        element.textContent = text;
    }

    return element;
}

function validate(node, stats = {count: 0}, depth = 0) {
    if (!node || Array.isArray(node) ||
        typeof node !== 'object' ||
        typeof node.type !== 'string' ||
        typeof node.label !== 'string' ||
        !Array.isArray(node.children)) {
        throw new Error(
            'Неправильний формат AST: очікуються type, label, children.'
        );
    }

    if (++stats.count > 500 || depth > 30) {
        throw new Error(
            'Дерево надто велике (максимум 500 вузлів, глибина 30).'
        );
    }

    node.children.forEach(child => validate(child, stats, depth + 1));

    return stats.count;
}

function childrenOf(node) {
    return collapsed.has(node) ? [] : node.children;
}

function collapseDeep(node, depth = 0) {
    if (depth >= 3 && node.children.length > 0) {
        collapsed.add(node);
    }

    node.children.forEach(child => collapseDeep(child, depth + 1));
}

function createLayout(root) {
    const nodes = [];
    const edges = [];

    let leaf = 0;
    let deepest = 0;

    function visit(node, depth, parent) {
        const item = {
            node,
            x: 0,
            y: 70 + 112 * depth
        };

        deepest = Math.max(deepest, depth);
        nodes.push(item);

        if (parent) {
            edges.push([parent, item]);
        }

        const descendants = childrenOf(node).map(
            child => visit(child, depth + 1, item)
        );

        if (descendants.length === 0) {
            item.x = 100 + leaf++ * 192;
        } else {
            item.x = (
                descendants[0].x +
                descendants[descendants.length - 1].x
            ) / 2;
        }

        return item;
    }

    visit(root, 0, null);

    return {
        nodes,
        edges,
        width: Math.max(500, leaf * 192 + 200),
        height: (deepest + 1) * 112 + 100
    };
}

function draw() {
    scene.replaceChildren();

    if (!layout) {
        return;
    }

    for (const [parent, child] of layout.edges) {
        const middle = (parent.y + child.y) / 2;

        const path =
            `M ${parent.x} ${parent.y + 28} ` +
            `C ${parent.x} ${middle}, ` +
            `${child.x} ${middle}, ` +
            `${child.x} ${child.y - 28}`;

        scene.appendChild(svgElement('path', {
            class: 'edge',
            d: path
        }));
    }

    for (const {node, x, y} of layout.nodes) {
        const group = svgElement('g', {
            class: `node ${node.type}`,
            transform: `translate(${x} ${y})`
        });

        group.appendChild(svgElement('rect', {
            x: -78,
            y: -30,
            width: 156,
            height: 60,
            rx: 12
        }));

        const shortLabel = node.label.length > 19
            ? node.label.slice(0, 17) + '…'
            : node.label;

        group.appendChild(svgElement('text', {
            y: -5,
            class: 'node-label'
        }, shortLabel));

        group.appendChild(svgElement('text', {
            y: 16,
            class: 'node-kind'
        }, node.type));

        group.appendChild(svgElement(
            'title', {}, `${node.label} (${node.type})`
        ));

        group.addEventListener('click', event => {
            event.stopPropagation();

            scene.querySelectorAll('.selected').forEach(
                element => element.classList.remove('selected')
            );

            group.classList.add('selected');
            detailsLabel.textContent = node.label;
            detailsType.textContent =
                `Тип: ${node.type} · Дочірніх вузлів: ${node.children.length}`;
        });

        if (node.children.length > 0) {
            const toggle = svgElement('g', {
                class: 'toggle',
                transform: 'translate(71 -25)'
            });

            toggle.appendChild(svgElement('circle', {r: 11}));

            toggle.appendChild(svgElement('text', {
                y: 4
            }, collapsed.has(node) ? '+' : '−'));

            toggle.appendChild(svgElement('title', {},
                collapsed.has(node)
                    ? 'Розгорнути гілку'
                    : 'Згорнути гілку'
            ));

            toggle.addEventListener('click', event => {
                event.stopPropagation();

                if (collapsed.has(node)) {
                    collapsed.delete(node);
                } else {
                    collapsed.add(node);
                }

                render();
            });

            group.appendChild(toggle);
        }

        scene.appendChild(group);
    }
}

function setView() {
    graph.setAttribute(
        'viewBox',
        `${view.x} ${view.y} ${view.w} ${view.h}`
    );
}

function fit() {
    if (!layout) {
        return;
    }

    const width = canvas.clientWidth || 1000;
    const height = canvas.clientHeight || 530;
    const aspect = width / height;

    view.w = Math.max(
        layout.width + 80,
        (layout.height + 80) * aspect
    );
    view.h = view.w / aspect;

    view.x = (layout.width - view.w) / 2;
    view.y = (layout.height - view.h) / 2;

    setView();
}

function render() {
    layout = createLayout(tree);
    draw();
    fit();
}

function loadTree(data, name) {
    const total = validate(data);

    tree = data;
    collapsed = new WeakSet();
    collapseDeep(tree);

    filename.textContent = name;
    nodeCount.textContent = `${total} вузлів`;

    detailsLabel.textContent = 'Натисни на вузол дерева';
    detailsType.textContent =
        'Значок +/− розгортає та згортає гілку.';

    render();

    status.textContent =
        'Дерево завантажено. Глибші гілки спочатку згорнуті.';
}

async function loadExample() {
    try {
        const response = await fetch(
            '../out/demo-ast.json',
            {cache: 'no-store'}
        );

        if (!response.ok) {
            throw new Error(
                'Не знайдено out/demo-ast.json. Виконай make generate-ast.'
            );
        }

        loadTree(await response.json(), 'out/demo-ast.json');
    } catch (error) {
        status.textContent =
            `Не вдалося завантажити приклад: ${error.message}`;
    }
}

fileInput.addEventListener('change', async () => {
    const file = fileInput.files[0];

    if (!file) {
        return;
    }

    try {
        loadTree(JSON.parse(await file.text()), file.name);
    } catch (error) {
        status.textContent = `Помилка файлу: ${error.message}`;
    }

    fileInput.value = '';
});

document.getElementById('exampleButton')
    .addEventListener('click', loadExample);

document.getElementById('fitButton')
    .addEventListener('click', fit);

document.getElementById('expandButton')
    .addEventListener('click', () => {
        if (!tree) return;

        collapsed = new WeakSet();
        render();
    });

document.getElementById('overviewButton')
    .addEventListener('click', () => {
        if (!tree) return;

        collapsed = new WeakSet();
        collapseDeep(tree);
        render();
    });

function zoom(factor, px = 0.5, py = 0.5) {
    const newWidth = Math.max(
        160,
        Math.min(15000, view.w * factor)
    );

    const ratio = newWidth / view.w;

    view.x += px * view.w * (1 - ratio);
    view.y += py * view.h * (1 - ratio);

    view.w *= ratio;
    view.h *= ratio;

    setView();
}

document.getElementById('zoomInButton')
    .addEventListener('click', () => zoom(0.8));

document.getElementById('zoomOutButton')
    .addEventListener('click', () => zoom(1.25));

graph.addEventListener('wheel', event => {
    event.preventDefault();

    const rectangle = graph.getBoundingClientRect();

    zoom(
        event.deltaY > 0 ? 1.12 : 0.89,
        (event.clientX - rectangle.left) / rectangle.width,
        (event.clientY - rectangle.top) / rectangle.height
    );
}, {passive: false});

graph.addEventListener('pointerdown', event => {
    if (event.button !== 0) return;
    if (event.target.closest('.node')) return;

    drag = {
        x: event.clientX,
        y: event.clientY,
        viewX: view.x,
        viewY: view.y
    };

    graph.setPointerCapture(event.pointerId);
});

graph.addEventListener('pointermove', event => {
    if (!drag) return;

    const rectangle = graph.getBoundingClientRect();

    view.x = drag.viewX -
        (event.clientX - drag.x) * view.w / rectangle.width;

    view.y = drag.viewY -
        (event.clientY - drag.y) * view.h / rectangle.height;

    setView();
});

graph.addEventListener('pointerup', () => {
    drag = null;
});

graph.addEventListener('pointercancel', () => {
    drag = null;
});

window.addEventListener('resize', fit);

loadExample();
