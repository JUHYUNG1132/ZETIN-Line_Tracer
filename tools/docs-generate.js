const fs = require("fs").promises;
const path = require('path');
const hljs = require('highlight.js');
const ketex = require('./libs/md-latex');
const markdown = require('markdown-it')({
    html: true,
    langPrefix: 'language-',
    linkify: false,
    typographer: false,
    highlight: (str, lang) => {
        if (lang && hljs.getLanguage(lang)) {
            try {
                return hljs.highlight(str, { language: lang }).value;
            } catch (_) { }
        }
        return ''; // use external default escaping
    }
});
markdown.use(ketex);

const DIR_MD = path.resolve(__dirname, '..', 'docs', 'lecture');
const PATH_INDEX = path.resolve(__dirname, '..', 'docs', 'index.html');

function parseMarkdown(markdownStr) {

    let toc = [];
    let tokens = markdown.parse(markdownStr, {});


    /**
     * This section does
     * - Assign an id to every header 
     * - Build table of content
     */
    let headerIndex = 1;
    for (let i = 0; i < tokens.length; i++) {
        let cur = tokens[i];
        let nxt = tokens[i + 1];

        if (cur.type === 'heading_open') {
            const id = `header-${headerIndex}`;
            const type = tokens[i].tag;
            const content = markdown.renderInline(nxt.content);

            if (!cur.attrs) cur.attrs = [];
            cur.attrs.push(['id', id]);

            toc.push({ type, content, id });
            headerIndex++;
        }
    }

    const html = markdown.renderer.render(tokens, markdown.options);
    return { html, toc };
}

function toc2html(toc) {
    const str = toc.map(header => {
        const content = header.content;
        return `<li class="toc-${header.type}"><a href="#${header.id}">${content}</a></li>`;
    }).join('');
    return `<ul class="toc">${str}</ul>`;
}

(async function main() {
    const htmlFileList = [];

    let lectureTemplate = await fs.readFile(path.join(__dirname, 'template-lecture.html'), { encoding: 'utf-8' });
    let indexTemplate = await fs.readFile(path.join(__dirname, 'template-index.html'), { encoding: 'utf-8' });

    let files = await fs.readdir(DIR_MD);
    await Promise.all(files
        .filter(file => path.extname(file) === '.md')
        .map(async file => {
            const mdFilePath = path.join(DIR_MD, file);
            const fileName = path.parse(file).name;
            const htmlFilePath = path.join(DIR_MD, fileName + '.html');

            let mdFile = await fs.readFile(mdFilePath, { encoding: 'utf-8' });
            let { html, toc } = parseMarkdown(mdFile);
            let tocHTML = toc2html(toc);
            html = html.replace('[TOC]', tocHTML);
            html = lectureTemplate.replace('[BODY]', html)
                .replace('[TITLE]', 'ZETIN::' + fileName)
                .replace('[UPDATE]', `최근 업데이트 : ${new Date()}`);

            await fs.writeFile(htmlFilePath, html, { encoding: 'utf-8' });

            htmlFileList.push(htmlFilePath);
        }));

    const listHTML = htmlFileList.map(file => {
        const relative = path.relative(path.dirname(PATH_INDEX), file);
        const name = path.parse(file).name;
        return `<li><a class="filename" href="${relative}">${name}</a> (최근 업데이트 : ${new Date()})</li>`;
    });

    const indexHTML = indexTemplate.replace('[LIST]', listHTML);
    await fs.writeFile(PATH_INDEX, indexHTML, { encoding: 'utf-8' });

    console.log("HTML generated.");
})();

