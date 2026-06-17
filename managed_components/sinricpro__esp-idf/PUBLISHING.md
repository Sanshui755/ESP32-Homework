# Publishing to ESP Component Registry

This guide explains how to publish the SinricPro ESP-IDF component to the official ESP Component Registry.

## Prerequisites

1. **ESP Component Registry Account**
   - Create an account at [components.espressif.com](https://components.espressif.com/)
   - Create a namespace (e.g., `sinricpro`)

2. **Generate API Token**
   - Go to [components.espressif.com/settings/api-keys](https://components.espressif.com/settings/api-keys)
   - Click "Generate New Token"
   - Copy the token (you'll only see it once!)

3. **Add Token to GitHub Secrets**
   - Go to your GitHub repository settings
   - Navigate to: Settings → Secrets and variables → Actions
   - Click "New repository secret"
   - Name: `IDF_COMPONENT_API_TOKEN`
   - Value: Paste your API token
   - Click "Add secret"

## How to Publish a New Release

### 1. Update Version

Edit `components/sinricpro/idf_component.yml` and update the version:

```yaml
version: "1.0.1"  # Update this
```

### 2. Commit and Push Changes

```bash
git add components/sinricpro/idf_component.yml
git commit -m "Bump version to 1.0.1"
git push origin main
```

### 3. Create GitHub Release

1. Go to your GitHub repository
2. Click on "Releases" (right sidebar)
3. Click "Draft a new release"
4. Fill in the release details:
   - **Tag**: `v1.0.1` (create new tag)
   - **Release title**: `v1.0.1` or `Release 1.0.1`
   - **Description**: Add release notes and changelog
5. Click "Publish release"

### 4. Automatic Upload

The GitHub workflow will automatically:
- Detect the new published release
- Upload the component to ESP Component Registry
- Component will be available at: `https://components.espressif.com/components/sinricpro/sinricpro`

You can monitor the upload progress in the "Actions" tab of your repository.

## Version Numbering

Follow [Semantic Versioning](https://semver.org/):
- **MAJOR** version (v**1**.0.0) - Breaking changes
- **MINOR** version (v1.**1**.0) - New features (backward compatible)
- **PATCH** version (v1.0.**1**) - Bug fixes (backward compatible)

## Manual Upload (Alternative)

If you prefer to upload manually:

```bash
cd components/sinricpro
compote component upload --namespace sinricpro --name sinricpro
```

## Verifying Upload

1. Visit [components.espressif.com](https://components.espressif.com/)
2. Search for "sinricpro"
3. Verify the new version appears
4. Check the documentation and examples are correct

## Troubleshooting

### Workflow Fails with "Authentication Failed"
- Verify the `IDF_COMPONENT_API_TOKEN` secret is set correctly
- Generate a new API token if needed

### Version Already Exists
- You cannot overwrite existing versions
- Increment the version number and create a new tag

### Component Not Found
- Verify the namespace matches your ESP Component Registry account
- Check the `idf_component.yml` is valid

## Resources

- [ESP Component Registry Documentation](https://docs.espressif.com/projects/idf-component-manager/)
- [Upload Components CI Action](https://github.com/espressif/upload-components-ci-action)
